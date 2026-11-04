/* ABYSS - CE front end. Compiles with CEdev's graphx/keypadc/fileioc (confirmed by earlier real builds of the same API subset);
   newly added calls are gfx_Sprite and timer_*. Never run on hardware by the author. */
#include <graphx.h>
#include <keypadc.h>
#include <tice.h>
#include <sys/timers.h>
#include <stdint.h>
#include "game.h"
#include "upgrades.h"
#include "abilities.h"
#include "animation.h"
#include "assets.h"
#include "platform.h"

#define SPR(a) ((gfx_sprite_t *)(a))
#define FRAME_TICKS 546   /* 32768 Hz / 60 */
enum { C_BG1 = 1, C_BG2, C_BG3, C_STONE_D, C_STONE, C_STONE_L, C_STONE_H, C_PURPLE, C_PURPLE_D, C_MOSS, C_MOSS_D, C_SKIN, C_SKIN_D,
       C_CLOAK, C_CLOAK_L, C_CLOAK_D, C_RUNE, C_RUNE_D, C_RED, C_RED_D, C_GOLD, C_GOLD_D, C_WHITE, C_BROWN, C_BROWN_D, C_BONE, C_BONE_D,
       C_OUT, C_FIRE, C_FIRE_L, C_GREEN };
static const char *const WORLD_NAMES[4] = {"FORGOTTEN CRYPT", "SUNKEN MINES", "INFERNAL DEPTHS", "THE VOID"};
static const char *const LEVEL_NAMES[LEVEL_COUNT] = {
  "The Entrance", "Forgotten Halls", "Burial Grounds", "Bone Chambers", "Bone King's Throne",
  "Mine Entrance", "Crystal Caverns", "Collapsing Tunnels", "Deep Shafts", "Broodmother's Nest",
  "Ashen Path", "Burning Halls", "Magma Bridge", "Furnace Chambers", "Inferno's Arena",
  "Forgotten Gate", "Shattered Reality", "Lost Kingdom", "Abyssal Core", "The Forgotten One"};

static int camx;
static uint16_t anim_tick;

/* ---------- helpers ---------- */
static void frame_wait(void) { while (timer_Get(1) < FRAME_TICKS) { } timer_Set(1, 0); }
static void present(void) { frame_wait(); gfx_SwapDraw(); anim_tick++; }
static void init_palette(void) {
  static uint16_t pal[32];
  for (int i = 0; i < 32; i++) pal[i] = gfx_RGBTo1555(PALETTE_RGB[i][0], PALETTE_RGB[i][1], PALETTE_RGB[i][2]);
  gfx_SetPalette(pal, sizeof pal, 0);
  gfx_SetTransparentColor(0);
}
static bool edge(bool now, bool *prev) { bool r = now && !*prev; *prev = now; return r; }
static Input read_input(void) {
  kb_Scan();
  Input in = { (kb_Data[7] & kb_Left) != 0, (kb_Data[7] & kb_Right) != 0, (kb_Data[7] & kb_Up) != 0, (kb_Data[7] & kb_Down) != 0,
               (kb_Data[1] & kb_2nd) != 0, (kb_Data[2] & kb_Alpha) != 0 };
  return in;
}
static void text(const char *s, int x, int y, uint8_t c) { gfx_SetTextFGColor(c); gfx_PrintStringXY(s, x, y); }
static void text_c(const char *s, int cx, int y, uint8_t c, int chars) { text(s, cx - chars * 4, y, c); }   /* approx. centred (8px font) */
static void num(int v, int x, int y, uint8_t c) { gfx_SetTextFGColor(c); gfx_SetTextXY(x, y); gfx_PrintUInt(v, 1); }
static void panel(int x, int y, int w, int h) {
  gfx_SetColor(C_OUT); gfx_FillRectangle(x, y, w, h); gfx_SetColor(C_STONE_L); gfx_Rectangle(x, y, w, h);
  gfx_SetColor(C_BG2); gfx_FillRectangle(x + 2, y + 2, w - 4, h - 4);
}
static int popcount32(uint32_t v) { int n = 0; while (v) { n += v & 1; v >>= 1; } return n; }
static int treasure_found(const SaveData *s, int lv) { return popcount32(s->treasure[lv]); }

/* ---------- backgrounds ---------- */
static void draw_background(int cam) {
  gfx_FillScreen(C_BG1);
  gfx_SetColor(C_BG2); gfx_FillRectangle(0, 70, 320, 170);
  for (int i = -1; i < 6; i++) {                                   /* far pillars, slow parallax */
    int bx = i * 72 - (cam / 3) % 72, h = 70 + ((i + cam / 216) & 3) * 30;
    gfx_SetColor(C_BG3); gfx_FillRectangle(bx, 240 - h, 24, h);
    gfx_SetColor(C_BG2); gfx_FillRectangle(bx + 4, 240 - h, 3, h);
    gfx_SetColor(C_STONE_D); gfx_FillRectangle(bx - 3, 240 - h, 30, 4);
  }
  gfx_SetColor(C_BG1); gfx_FillRectangle(0, 0, 320, 18);
}

/* ---------- world rendering ---------- */
static void draw_tiles(const Game *g) {
  const LevelState *l = &g->ls; const LevelDef *d = l->def; int t0 = camx / TILE_PX;
  for (int ty = 0; ty < d->h; ty++)
    for (int tx = t0; tx <= t0 + 20 && tx < d->w; tx++) {
      uint8_t t = level_tile(l, tx, ty); if (!t) continue;
      int x = tx * TILE_PX - camx, y = ty * TILE_PX; int s = -1; bool opaque = false;
      switch (t) {
        case T_SOLID: { uint8_t up = level_tile(l, tx, ty - 1); s = (up == T_SOLID || up == T_GATE || up == T_CRUMBLE) ? TS_SOLID : TS_SOLID_TOP; opaque = true; } break;
        case T_CRUMBLE: s = TS_CRUMBLE; opaque = true; break;
        case T_ONEWAY: s = TS_ONEWAY; break;
        case T_SPIKE: s = TS_SPIKE; break;
        case T_LADDER: s = TS_LADDER; break;
        case T_COIN: s = TS_COIN0 + ((anim_tick / 6 + tx) & 3); break;
        case T_CHEST: s = TS_CHEST; break;
        case T_EXIT: s = TS_EXIT; break;
        case T_RUNE: s = TS_RUNE_OFF; break;
        case T_RUNE_ON: s = TS_RUNE_ON; break;
        case T_GATE: s = TS_GATE; break;
        case T_TORCH: s = TS_TORCH0 + ((anim_tick / 8 + tx) & 1); break;
        case T_SKULL: s = TS_SKULL; break;
        default: break;
      }
      if (s < 0) continue;
      if (opaque) gfx_Sprite(SPR(TILE_SPR[s]), x, y); else gfx_TransparentSprite(SPR(TILE_SPR[s]), x, y);
    }
}
static void draw_entities(const Game *g) {
  const LevelState *l = &g->ls; const LevelDef *d = l->def;
  if (d->boss && !l->gates_open) {                                      /* The Bone King looms over the arena */
    int kx = d->w * 8 - 20 - camx, ky = 22 + ((anim_tick / 10) & 1) * 2;
    gfx_TransparentSprite(SPR(KING_SPR[(anim_tick / 24) & 1]), kx, ky);
  }
  for (int i = 0; i < l->nplats; i++) {
    int x = l->plats[i].x - camx, y = l->plats[i].y;
    gfx_SetColor(C_OUT); gfx_FillRectangle(x - 1, y - 1, PLAT_W + 2, 8);
    gfx_SetColor(C_PURPLE_D); gfx_FillRectangle(x, y, PLAT_W, 6);
    gfx_SetColor(C_PURPLE); gfx_HorizLine_NoClip(x, y, PLAT_W); gfx_SetColor(C_STONE_H);
    gfx_FillRectangle(x + 3, y + 2, 2, 2); gfx_FillRectangle(x + 15, y + 2, 2, 2); gfx_FillRectangle(x + 27, y + 2, 2, 2);
  }
  for (int i = 0; i < g->ncr; i++) {
    const Crawler *c = &g->cr[i];
    gfx_TransparentSprite(SPR(CRAWLER_SPR[c->dir > 0 ? 0 : 1][(anim_tick / 8) & 1]), c->x - camx, c->y);
  }
  for (int i = 0; i < MAX_BONES; i++) {
    const Bone *b = &g->bone[i]; if (!b->on) continue;
    if (b->t) { if ((b->t / 3) & 1) { gfx_SetColor(C_RED); gfx_FillRectangle(b->x - camx, 17, 6, 3); } }
    else gfx_TransparentSprite(SPR(BONE_SPR), b->x - camx, b->y);
  }
  if (!(g->invuln & 4)) {
    int f = anim_player_frame(&g->p, g->tick);
    gfx_TransparentSprite(SPR(PLAYER_SPR[g->p.face ? 0 : 1][f]), g->p.x / SUBPX - 1 - camx, g->p.y / SUBPX - 2);
  }
  if (g->pulse_fx) {                                                    /* Rune Pulse: expanding ring */
    int r = (12 - g->pulse_fx) * 6 + 6, cx = g->p.x / SUBPX + PW / 2 - camx, cy = g->p.y / SUBPX + PH / 2;
    gfx_SetColor(C_RUNE); gfx_Rectangle(cx - r, cy - r, r * 2, r * 2); gfx_SetColor(C_WHITE); gfx_Rectangle(cx - r + 2, cy - r + 2, r * 2 - 4, r * 2 - 4);
  }
}
static void draw_hud(const Game *g, const SaveData *s) {
  const LevelDef *d = g->ls.def; int mh = upgrade_max_hp(s), me = upgrade_max_energy(s);
  gfx_TransparentSprite(SPR(HEART_SPR), 4, 4);
  gfx_SetColor(C_OUT); gfx_FillRectangle(15, 4, 82, 9);
  gfx_SetColor(C_RED_D); gfx_FillRectangle(16, 5, 80, 7); gfx_SetColor(g->hp * 4 < mh ? C_FIRE : C_RED); gfx_FillRectangle(16, 5, g->hp * 80 / mh, 7);
  gfx_SetColor(C_OUT); gfx_FillRectangle(15, 14, 82, 6);
  gfx_SetColor(C_CLOAK_D); gfx_FillRectangle(16, 15, 80, 4); gfx_SetColor(C_RUNE); gfx_FillRectangle(16, 15, g->energy * 80 / me, 4);
  gfx_TransparentSprite(SPR(TILE_SPR[TS_COIN0 + ((anim_tick / 6) & 3)]), 262, 1);
  num(s->coins, 280, 5, C_GOLD);
  if (d->boss) { text("RUNES", 112, 5, C_BONE); num(g->ls.runes_on, 160, 5, C_RUNE); text("/3", 168, 5, C_RUNE); }
  else if (g->ls.runes_total) { text("RUNES", 112, 5, C_BONE); num(g->ls.runes_on, 160, 5, C_RUNE); gfx_PrintString("/"); gfx_PrintUInt(g->ls.runes_total, 1); }
  if (g->ls.runes_total && g->ls.gates_open) text("GATE OPEN", 190, 5, C_GREEN);
}
static void render(const Game *g, const SaveData *s) {
  const LevelDef *d = g->ls.def; int target = g->p.x / SUBPX + PW / 2 - 160 + (g->p.face ? 24 : -24), maxcam = d->w * TILE_PX - 320;
  if (target > maxcam) target = maxcam;
  if (target < 0) target = 0;
  if (g->tick <= 1) camx = target; else camx += (target - camx) / 4;
  draw_background(camx); draw_tiles(g); draw_entities(g); draw_hud(g, s);
  if (g->tick < 110) {                                                   /* level banner */
    panel(60, 190, 200, 34); text_c(d->name, 160, 196, C_GOLD, (int)__builtin_strlen(d->name));
    text_c(g->ls.def->boss ? "GUARDIAN: light all runes!" : "Reach the exit", 160, 210, C_STONE_H, g->ls.def->boss ? 25 : 14);
  }
}

/* ---------- screens ---------- */
static bool pause_menu(void) {                      /* returns true to leave the level */
  bool pe = false, pc = true;
  for (;;) {
    kb_Scan();
    if (edge(kb_Data[6] & kb_Clear, &pc)) return false;
    if (edge(kb_Data[6] & kb_Enter, &pe)) return true;
    panel(70, 90, 180, 60); text_c("PAUSED", 160, 98, C_GOLD, 6);
    text_c("CLEAR  resume", 160, 116, C_WHITE, 13); text_c("ENTER  leave level", 160, 130, C_WHITE, 18);
    present();
  }
}
static bool screen_complete(const Game *g, const SaveData *s, bool has_next) {   /* returns true to continue to the next level */
  bool pe = false, pc = false; const LevelDef *d = g->ls.def; int lv = g->level; kb_Scan();
  pe = kb_Data[6] & kb_Enter; pc = kb_Data[6] & kb_Clear;
  for (int t = 0;; t++) {
    kb_Scan();
    if (edge(kb_Data[6] & kb_Enter, &pe) && t > 20) return has_next;
    if (edge(kb_Data[6] & kb_Clear, &pc) && t > 20) return false;
    draw_background(camx); draw_tiles(g); draw_hud(g, s);
    panel(40, 50, 240, 140);
    text_c(d->boss ? "GUARDIAN DEFEATED!" : "LEVEL COMPLETE", 160, 60, d->boss ? C_RUNE : C_GREEN, d->boss ? 18 : 14);
    text_c(d->name, 160, 80, C_GOLD, (int)__builtin_strlen(d->name));
    text("Coins gained", 60, 104, C_WHITE); num(g->run_coins, 200, 104, C_GOLD);
    text("Treasures", 60, 120, C_WHITE); num(treasure_found(s, lv), 200, 120, C_GOLD); gfx_PrintString(" / "); gfx_PrintUInt(level_treasure_count(d), 1);
    text("Deaths (total)", 60, 136, C_WHITE); num(s->deaths, 200, 136, C_STONE_H);
    if (d->boss) text_c("WORLD 1 CLEARED - more worlds soon", 160, 156, C_RUNE, 34);
    text_c(has_next ? "ENTER next level   CLEAR map" : "ENTER / CLEAR  back to map", 160, 172, C_STONE_H, has_next ? 29 : 26);
    present();
  }
}
/* Plays a level. Returns true if the player finished it. */
static bool play_level(SaveData *s, int idx, int *next_unlocked_play) {
  Game g; game_start_level(&g, s, idx); bool pc = true; (void)next_unlocked_play;
  timer_Set(1, 0);
  for (;;) {
    Input in = read_input();
    if (edge(kb_Data[6] & kb_Clear, &pc)) { if (pause_menu()) { platform_save_write(s); return false; } timer_Set(1, 0); }
    uint8_t ev = game_update(&g, in);
    if (ev & EV_WIN) { platform_save_write(s); render(&g, s); (void)ev; return screen_complete(&g, s, idx + 1 < LEVELS_IMPLEMENTED) ? true : false; }
    render(&g, s); present();
  }
}
static void screen_levels(SaveData *s) {
  int sel = s->unlocked < LEVELS_IMPLEMENTED ? s->unlocked : LEVELS_IMPLEMENTED - 1; bool pl = true, pr = true, pu = true, pd = true, pe = true, pc = true;
  for (;;) {
    kb_Scan();
    if (edge(kb_Data[6] & kb_Clear, &pc)) return;
    if (edge(kb_Data[7] & kb_Left, &pl) && sel > 0) sel--;
    if (edge(kb_Data[7] & kb_Right, &pr) && sel < LEVEL_COUNT - 1) sel++;
    if (edge(kb_Data[7] & kb_Up, &pu) && sel >= 5) sel -= 5;
    if (edge(kb_Data[7] & kb_Down, &pd) && sel < LEVEL_COUNT - 5) sel += 5;
    bool avail = sel < LEVELS_IMPLEMENTED, open = avail && sel <= s->unlocked;
    if (edge(kb_Data[6] & kb_Enter, &pe) && open) {
      int cur = sel;
      while (play_level(s, cur, 0)) { cur++; if (cur >= LEVELS_IMPLEMENTED) break; }
      sel = (cur < LEVELS_IMPLEMENTED && cur <= s->unlocked) ? cur : (cur >= LEVELS_IMPLEMENTED ? LEVELS_IMPLEMENTED - 1 : sel);
      pe = true; pc = true;
      continue;
    }
    draw_background(0); text_c("SELECT LEVEL", 160, 5, C_GOLD, 12);
    for (int w = 0; w < 4; w++) {
      int y = 22 + w * 50;
      gfx_SetColor(w == sel / 5 ? C_STONE_L : C_STONE_D); gfx_HorizLine(8, y, 304);
      text(WORLD_NAMES[w], 10, y + 3, w == sel / 5 ? C_GOLD : C_STONE_L);
      for (int k = 0; k < 5; k++) {
        int i = w * 5 + k, x = 12 + k * 61, by = y + 16; bool done = (s->completed >> i) & 1, ok = i < LEVELS_IMPLEMENTED && i <= s->unlocked;
        gfx_SetColor(C_OUT); gfx_FillRectangle(x, by, 54, 26);
        gfx_SetColor(done ? C_GREEN : ok ? C_STONE_L : C_STONE_D); gfx_FillRectangle(x + 1, by + 1, 52, 24);
        gfx_SetColor(done ? C_MOSS_D : ok ? C_STONE : C_BG3); gfx_FillRectangle(x + 3, by + 3, 48, 20);
        if (i == sel) { gfx_SetColor(C_GOLD); gfx_Rectangle(x - 2, by - 2, 58, 30); gfx_Rectangle(x - 1, by - 1, 56, 28); }
        gfx_SetTextFGColor(ok ? C_WHITE : C_STONE); gfx_SetTextXY(x + (i + 1 < 10 ? 20 : 16), by + 9); gfx_PrintUInt(i + 1, 1);
        if (k == 4) gfx_TransparentSprite(SPR(HEART_SPR), x + 40, by + 4);       /* guardian marker */
        if (done) text("OK", x + 4, by + 4, C_GOLD);
        else if (!ok) text(i < LEVELS_IMPLEMENTED ? "LOCK" : "SOON", x + 4, by + 4, C_STONE_L);
      }
    }
    panel(8, 226 - 12, 304, 14);
    text(LEVEL_NAMES[sel], 12, 217, C_GOLD);
    if (!avail) text("Coming soon", 180, 217, C_STONE_L);
    else if (!open) text("Locked", 180, 217, C_RED);
    else { text("ENTER play", 180, 217, C_GREEN); gfx_PrintString(" "); }
    present();
  }
}
static void screen_upgrades(SaveData *s) {
  static const char *N[3] = {"ARMOR", "MAX HEALTH", "ENERGY"}; static const char *D[3] = {"-2 damage per level", "+20 max HP per level", "+20 energy per level"};
  int sel = 0; bool pu = true, pd = true, pe = true, pc = true; int flash = 0;
  for (;;) {
    kb_Scan();
    if (edge(kb_Data[6] & kb_Clear, &pc)) { platform_save_write(s); return; }
    if (edge(kb_Data[7] & kb_Up, &pu) && sel > 0) sel--;
    if (edge(kb_Data[7] & kb_Down, &pd) && sel < 2) sel++;
    if (edge(kb_Data[6] & kb_Enter, &pe)) { flash = upgrade_buy(s, (UpgCat)sel) == BUY_OK ? 12 : -12; }
    draw_background(0); text_c("UPGRADES", 160, 8, C_GOLD, 8);
    gfx_TransparentSprite(SPR(TILE_SPR[TS_COIN0 + ((anim_tick / 6) & 3)]), 110, 26); num(s->coins, 130, 31, C_GOLD); gfx_PrintString(" coins");
    for (int i = 0; i < 3; i++) {
      int y = 56 + i * 52; panel(20, y, 280, 44);
      if (i == sel) { gfx_SetColor(C_GOLD); gfx_Rectangle(18, y - 2, 284, 48); }
      text(N[i], 30, y + 6, i == sel ? C_GOLD : C_WHITE); text(D[i], 30, y + 24, C_STONE_L);
      for (int p = 0; p < UPG_MAX_LEVEL; p++) { gfx_SetColor(C_OUT); gfx_FillRectangle(170 + p * 16, y + 8, 13, 10); gfx_SetColor(p < s->upg[i] ? C_RUNE : C_STONE_D); gfx_FillRectangle(171 + p * 16, y + 9, 11, 8); }
      int c = upgrade_cost((UpgCat)i, s->upg[i]);
      if (c < 0) text("MAXED", 190, y + 26, C_GREEN); else { text("Cost", 190, y + 26, C_WHITE); num(c, 230, y + 26, s->coins >= (uint32_t)c ? C_GOLD : C_RED); }
    }
    if (flash > 0) { text_c("UPGRADED!", 160, 222, C_GREEN, 9); flash--; } else if (flash < 0) { text_c("Not available", 160, 222, C_RED, 13); flash++; }
    else text_c("UP/DOWN select   ENTER buy   CLEAR back", 160, 222, C_STONE_L, 38);
    present();
  }
}
static void screen_info(const SaveData *s, bool collection) {
  bool pc = true; int done = 0, found = 0, total = 0;
  for (int i = 0; i < LEVEL_COUNT; i++) done += (s->completed >> i) & 1;
  for (int i = 0; i < LEVELS_IMPLEMENTED; i++) { found += treasure_found(s, i); total += level_treasure_count(&LEVELS[i]); }
  for (;;) {
    kb_Scan(); if (edge(kb_Data[6] & kb_Clear, &pc)) return;
    draw_background(0); text_c(collection ? "COLLECTION" : "STATS", 160, 8, C_GOLD, collection ? 10 : 5);
    panel(20, 28, 280, 190);
    if (!collection) {
      text("Levels completed", 34, 42, C_WHITE); num(done, 230, 42, C_GOLD); gfx_PrintString(" / 20");
      text("Guardians defeated", 34, 62, C_WHITE); num(popcount32(s->guardians), 230, 62, C_GOLD); gfx_PrintString(" / 4");
      text("Total deaths", 34, 82, C_WHITE); num(s->deaths, 230, 82, C_RED);
      text("Treasures found", 34, 102, C_WHITE); num(found, 230, 102, C_GOLD); gfx_PrintString(" / "); gfx_PrintUInt(total, 1);
      text("Secret chests", 34, 122, C_WHITE); num(s->secrets, 230, 122, C_GOLD);
      text("Unlocked level", 34, 142, C_WHITE); num(s->unlocked + 1, 230, 142, C_GOLD);
    } else {
      for (int i = 0; i < ABILITY_COUNT; i++) {
        bool have = (s->abilities >> i) & 1; int y = 36 + i * 14;
        text(have ? ABILITY_NAMES[i] : "??????", 34, y, have ? C_RUNE : C_STONE);
        text(have ? "unlocked" : "level", 200, y, have ? C_GREEN : C_STONE); if (!have) { gfx_PrintString(" "); gfx_PrintUInt(ABILITY_LEVEL[i], 1); }
      }
      int pct = (done + found) * 100 / (LEVELS_IMPLEMENTED + total);
      text("Treasure records", 34, 142, C_WHITE); num(found, 230, 142, C_GOLD); gfx_PrintString("/"); gfx_PrintUInt(total, 1);
      text("Completion (built content)", 34, 160, C_WHITE); num(pct, 250, 160, C_GOLD); gfx_PrintString("%");
      text("Relics: not implemented yet", 34, 190, C_STONE_L);
    }
    present();
  }
}
/* ---------- title / Waystation ---------- */
typedef struct { int x, y, v; } Ember;
int main(void) {
  static SaveData save; platform_save_load(&save);
  gfx_Begin(); gfx_SetDrawBuffer(); init_palette(); gfx_SetTextScale(1, 1);
  timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP); timer_Set(1, 0);
  static const char *M[4] = {"PLAY", "UPGRADES", "COLLECTION", "STATS"};
  static Ember em[24]; for (int i = 0; i < 24; i++) em[i] = (Ember){(i * 53) % 320, (i * 37) % 240, 1 + i % 3};
  int sel = 0; bool pu = true, pd = true, pe = true;
  for (;;) {
    kb_Scan(); if (kb_Data[6] & kb_Clear) break;
    if (edge(kb_Data[7] & kb_Up, &pu) && sel > 0) sel--;
    if (edge(kb_Data[7] & kb_Down, &pd) && sel < 3) sel++;
    if (edge(kb_Data[6] & kb_Enter, &pe)) {
      if (sel == 0) screen_levels(&save); else if (sel == 1) screen_upgrades(&save); else screen_info(&save, sel == 2);
      timer_Set(1, 0);
    }
    draw_background(anim_tick);
    for (int i = 0; i < 24; i++) { em[i].y -= em[i].v; em[i].x += (anim_tick / 16 + i) % 3 - 1; if (em[i].y < 0) { em[i].y = 240; em[i].x = (i * 97 + anim_tick) % 320; }
      gfx_SetColor(em[i].v == 3 ? C_FIRE_L : em[i].v == 2 ? C_FIRE : C_RUNE_D); gfx_FillRectangle(em[i].x, em[i].y, em[i].v == 3 ? 2 : 1, em[i].v == 3 ? 2 : 1); }
    for (int i = 0; i < 20; i++) { gfx_Sprite(SPR(TILE_SPR[TS_SOLID_TOP]), i * 16, 224); }
    gfx_TransparentSprite(SPR(TILE_SPR[TS_TORCH0 + ((anim_tick / 8) & 1)]), 40, 208); gfx_TransparentSprite(SPR(TILE_SPR[TS_TORCH0 + ((anim_tick / 8 + 1) & 1)]), 264, 208);
    gfx_TransparentSprite(SPR(PLAYER_SPR[0][FR_IDLE0 + ((anim_tick / 28) & 1)]), 154, 208);
    gfx_SetTextScale(4, 4); text("ABYSS", 82, 18, C_OUT); text("ABYSS", 80, 16, C_RUNE); text("ABYSS", 80, 14, C_RUNE_D); gfx_SetTextScale(1, 1);
    text_c("THE WAYSTATION", 160, 62, C_STONE_H, 14);
    panel(100, 82, 120, 108);
    for (int i = 0; i < 4; i++) {
      int y = 92 + i * 24;
      if (i == sel) { gfx_SetColor(C_PURPLE_D); gfx_FillRectangle(104, y - 4, 112, 18); gfx_SetColor(C_GOLD); gfx_Rectangle(104, y - 4, 112, 18); }
      text(M[i], 160 - (int)__builtin_strlen(M[i]) * 4, y, i == sel ? C_GOLD : C_WHITE);
    }
    text("CLEAR quit", 6, 230, C_STONE_L);
    present();
  }
  platform_save_write(&save);
  gfx_End(); return 0;
}
