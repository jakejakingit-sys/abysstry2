#!/usr/bin/env python3
"""Procedural pixel-art generator -> src/assets.c  (indexed 8-bit, index 0 = transparent, 32-colour palette)."""
import math
PAL = [  # index: (name, r,g,b)
 ('clear',0,0,0),('bg1',8,10,24),('bg2',18,22,48),('bg3',30,36,70),('stone_d',50,54,74),('stone',84,90,116),
 ('stone_l',128,136,160),('stone_h',170,178,196),('purple',96,64,128),('purple_d',56,36,80),('moss',110,160,120),('moss_d',62,100,76),
 ('skin',236,200,160),('skin_d',190,150,120),('cloak',54,74,170),('cloak_l',90,120,220),('cloak_d',30,40,100),('rune',90,240,240),
 ('rune_d',30,140,160),('red',220,50,50),('red_d',120,20,30),('gold',255,214,70),('gold_d',180,130,30),('white',255,255,255),
 ('brown',130,86,44),('brown_d',80,50,24),('bone',230,226,200),('bone_d',170,164,140),('outline',10,10,22),('fire',255,150,40),
 ('fire_l',255,230,120),('green',80,200,130)]
I = {n: i for i, (n, *_ ) in enumerate(PAL)}
def H(x, y, s=0): return ((x * 73856093) ^ (y * 19349663) ^ (s * 83492791)) & 0xFFFF

class C:
    def __init__(s, w, h): s.w, s.h = w, h; s.p = [[0] * w for _ in range(h)]
    def px(s, x, y, c):
        if 0 <= x < s.w and 0 <= y < s.h: s.p[y][x] = I[c] if isinstance(c, str) else c
    def rect(s, x, y, w, h, c):
        for j in range(h):
            for i in range(w): s.px(x + i, y + j, c)
    def hline(s, x, y, w, c): s.rect(x, y, w, 1, c)
    def vline(s, x, y, h, c): s.rect(x, y, 1, h, c)
    def ellipse(s, cx, cy, rx, ry, c):
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            for x in range(int(cx - rx - 1), int(cx + rx + 2)):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0: s.px(x, y, c)
    def outline(s, c='outline'):
        o = [r[:] for r in s.p]
        for y in range(s.h):
            for x in range(s.w):
                if s.p[y][x] == 0: continue
                for dx, dy in ((1,0),(-1,0),(0,1),(0,-1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < s.w and 0 <= ny < s.h and s.p[ny][nx] == 0: pass
                # edge pixels: recolour only if 4-neighbour transparent
        for y in range(s.h):
            for x in range(s.w):
                if s.p[y][x] != 0: continue
                for dx, dy in ((1,0),(-1,0),(0,1),(0,-1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < s.w and 0 <= ny < s.h and s.p[ny][nx] not in (0, I[c]): o[y][x] = I[c]
        s.p = o
    def flip(s):
        n = C(s.w, s.h); n.p = [r[::-1] for r in s.p]; return n
    def data(s): return [s.w, s.h] + [v for r in s.p for v in r]

# ---------------- tiles ----------------
def solid(top=False, crumble=False, s=1):
    c = C(16, 16); base = 'stone_l' if crumble else 'stone'
    c.rect(0, 0, 16, 16, base)
    for r in (0, 8):
        c.hline(0, r + 7, 16, 'stone_d'); c.hline(0, r, 16, 'stone_h' if crumble else 'stone_l')
        xs = (7, 15) if r == 0 else (3, 11)
        for x in xs: c.vline(x, r, 8, 'stone_d')
    for y in range(16):
        for x in range(16):
            v = H(x, y, s)
            if v % 11 == 0: c.px(x, y, 'stone_d')
            elif v % 17 == 0: c.px(x, y, 'stone_l' if not crumble else 'stone_h')
    if top:
        for x in range(16):
            d = 2 + (H(x, 0, s) % 3 == 0) + (H(x, 1, s) % 5 == 0)
            for y in range(d): c.px(x, y, 'moss' if y < d - 1 else 'moss_d')
    if crumble:
        for (x, y) in [(3,1),(4,2),(4,3),(5,4),(6,5),(6,6),(7,7),(7,8),(8,9),(9,10),(9,11),(11,4),(10,5),(10,6),(9,7),(12,10),(12,11),(13,12)]:
            c.px(x, y, 'outline')
        for (x, y) in [(0,0),(15,0),(0,15),(15,15),(1,0),(0,1)]: c.px(x, y, 'stone_d')
    return c
def oneway():
    c = C(16, 16)
    c.rect(0, 0, 16, 5, 'purple_d'); c.hline(0, 0, 16, 'purple'); c.hline(0, 1, 16, 'stone_l'); c.hline(0, 4, 16, 'outline')
    for x in (2, 8, 14): c.px(x, 2, 'stone_h')
    return c
def spike():
    c = C(16, 16)
    for i in range(3):
        cx = 3 + i * 5
        for y in range(5, 16):
            w = 1 + (y - 5) * 4 // 10; x0 = cx - w // 2
            for x in range(x0, x0 + w): c.px(x, y, 'stone_l' if x >= cx else 'stone_h')
        c.px(cx, 5, 'red'); c.px(cx, 6, 'red_d')
    c.hline(0, 15, 16, 'stone_d')
    return c
def ladder():
    c = C(16, 16)
    for x in (3, 11): c.vline(x, 0, 16, 'brown'); c.vline(x + 1, 0, 16, 'brown_d')
    for y in (2, 7, 12): c.hline(3, y, 10, 'brown'); c.hline(3, y + 1, 10, 'brown_d')
    return c
def coin(f):
    c = C(16, 16); hw = [5, 3, 1, 3][f]
    c.ellipse(8, 8, hw + .5, 5.5, 'gold_d'); c.ellipse(8, 8, max(hw - 1, .5), 4.5, 'gold')
    if f in (0, 3): c.px(6, 5, 'white'); c.px(6, 6, 'fire_l')
    return c
def chest():
    c = C(16, 16)
    c.rect(1, 7, 14, 9, 'brown'); c.rect(1, 5, 14, 4, 'brown_d'); c.hline(2, 4, 12, 'brown_d')
    c.hline(1, 7, 14, 'outline'); c.vline(1, 5, 11, 'outline'); c.vline(14, 5, 11, 'outline'); c.hline(1, 15, 14, 'outline')
    for x in (3, 11): c.vline(x, 4, 12, 'gold_d')
    c.rect(7, 8, 2, 3, 'gold'); c.px(7, 8, 'fire_l'); c.hline(2, 5, 12, 'brown')
    return c
def door():
    c = C(16, 16); c.rect(1, 0, 14, 16, 'stone_l'); c.rect(2, 0, 12, 16, 'stone')
    c.rect(3, 3, 10, 13, 'green'); c.rect(4, 2, 8, 1, 'green'); c.rect(5, 1, 6, 1, 'green')
    for y in range(3, 16): c.hline(3, y, 10, 'green' if y % 2 else 'rune_d')
    c.rect(6, 5, 4, 11, 'white'); c.rect(7, 4, 2, 1, 'white')
    for (x, y) in [(4,5),(11,8),(5,12),(10,4)]: c.px(x, y, 'white')
    c.vline(1, 0, 16, 'outline'); c.vline(14, 0, 16, 'outline')
    return c
def rune(on):
    c = C(16, 16)
    c.rect(4, 12, 8, 4, 'stone'); c.hline(4, 12, 8, 'stone_h'); c.vline(4, 12, 4, 'outline'); c.vline(11, 12, 4, 'outline')
    col = 'rune' if on else 'purple'; dk = 'rune_d' if on else 'purple_d'
    for (x, y) in [(8,2),(7,3),(9,3),(6,4),(10,4),(5,5),(11,5),(6,6),(10,6),(7,7),(9,7),(8,8)]: c.px(x, y, col)
    for (x, y) in [(8,4),(8,5),(8,6),(7,5),(9,5)]: c.px(x, y, dk)
    c.vline(8, 9, 3, dk)
    if on:
        for (x, y) in [(3,4),(13,4),(4,8),(12,8),(8,0)]: c.px(x, y, 'white')
        c.px(8, 5, 'white')
    return c
def gate():
    c = C(16, 16)
    for x in (1, 6, 11): c.rect(x, 0, 3, 16, 'stone_d'); c.vline(x, 0, 16, 'stone_l'); c.vline(x + 2, 0, 16, 'outline')
    c.rect(0, 3, 16, 2, 'purple'); c.rect(0, 11, 16, 2, 'purple'); c.hline(0, 3, 16, 'stone_h'); c.hline(0, 11, 16, 'stone_h')
    for y in (4, 12): c.px(7, y, 'rune'); c.px(8, y, 'rune')
    return c
def torch(f):
    c = C(16, 16); c.rect(7, 8, 2, 8, 'brown'); c.hline(6, 8, 4, 'brown_d')
    h = 6 if f == 0 else 5; sh = 0 if f == 0 else 1
    c.ellipse(8 + sh, 5, 3, h / 2 + 1, 'fire'); c.ellipse(8 + sh, 6, 1.5, 2.5, 'fire_l')
    c.px(8 - sh, 1, 'fire'); 
    return c
def skull():
    c = C(16, 16)
    c.ellipse(8, 11, 4, 3.5, 'bone'); c.rect(6, 13, 5, 2, 'bone'); c.hline(6, 15, 5, 'bone_d')
    c.rect(5, 10, 2, 2, 'outline'); c.rect(9, 10, 2, 2, 'outline'); c.px(8, 12, 'outline')
    for x in (6, 8, 10): c.px(x, 14, 'outline')
    c.hline(1, 15, 4, 'bone'); c.hline(12, 15, 3, 'bone_d'); c.px(0, 14, 'bone'); c.px(15, 14, 'bone')
    return c

# ---------------- player ----------------
BODY = ["...oooooo...", "..occCCccco.", ".occcccccco.", ".occcssrsco.", ".occcssssco.", "..occcSSco..",
        ".occggggcco.", ".occccrccco.", ".occccRccco.", ".obbbbbbbbo.", ".occcdddccco"]
LEG = {'.': 0, 'o': I['outline'], 'B': I['brown_d']}
CH = {'.':0,'o':'outline','c':'cloak','C':'cloak_l','d':'cloak_d','s':'skin','S':'skin_d','r':'rune','R':'rune_d','g':'gold','b':'brown','B':'brown_d'}
def leg(c, x, h, bottom=15):
    top = bottom - h + 1
    c.rect(x, top, 4, h, 'outline'); c.rect(x + 1, top, 2, h - 1, 'brown_d'); c.px(x + 4, bottom, 'outline')
def player(name):
    c = C(12, 16)
    bob = 1 if name in ('idle1', 'run1', 'run3') else 0
    for y, row in enumerate(BODY):
        for x, ch in enumerate(row):
            if ch != '.': c.px(x, y + bob, CH[ch])
    L = {'idle0': [(3,5,15),(7,5,15)], 'idle1': [(3,5,15),(7,5,15)], 'run0': [(2,5,15),(7,3,13)], 'run1': [(4,5,15),(6,5,15)],
         'run2': [(7,5,15),(2,3,13)], 'run3': [(4,5,15),(6,5,15)], 'jump': [(3,3,13),(7,3,13)], 'fall': [(2,5,15),(8,5,15)],
         'climb0': [(3,5,15),(7,3,13)], 'climb1': [(3,3,13),(7,5,15)]}[name]
    for (x, h, b) in L: leg(c, x, h, b)
    return c
PFRAMES = ['idle0','idle1','run0','run1','run2','run3','jump','fall','climb0','climb1']

# ---------------- creatures ----------------
def crawler(f):
    c = C(14, 10)
    spans = [(4,9),(2,11),(1,12),(1,12),(1,12),(1,12),(1,12)]
    for y, (a, b) in enumerate(spans):
        for x in range(a, b + 1): c.px(x, y + 1, 'bone_d' if y >= 5 else 'bone')
    for x in range(3, 10): c.px(x, 1, 'bone')
    c.px(10, 3, 'red'); c.px(11, 3, 'red'); c.px(10, 4, 'red_d'); c.hline(9, 6, 4, 'outline')
    for (x, y) in [(4,3),(5,4),(3,4)]: c.px(x, y, 'bone_d')
    off = 0 if f == 0 else 1
    for x in (2, 6, 10): c.vline(x + off, 8, 2, 'outline'); c.px(x + off + 1, 9, 'outline')
    c.outline(); return c
def bone():
    c = C(6, 12)
    c.rect(1, 1, 1, 2, 'bone'); c.rect(3, 1, 1, 2, 'bone'); c.rect(2, 2, 1, 8, 'bone'); c.rect(1, 9, 1, 2, 'bone'); c.rect(3, 9, 1, 2, 'bone')
    c.vline(2, 3, 6, 'bone_d'); c.px(2, 2, 'bone'); c.outline(); return c
def king(f):
    c = C(40, 40)
    # arms
    if f == 0: c.rect(2, 26, 3, 12, 'bone_d'); c.rect(35, 26, 3, 12, 'bone_d'); c.rect(1, 36, 5, 3, 'bone'); c.rect(34, 36, 5, 3, 'bone')
    else: c.rect(2, 12, 3, 16, 'bone_d'); c.rect(35, 12, 3, 16, 'bone_d'); c.rect(1, 9, 5, 4, 'bone'); c.rect(34, 9, 5, 4, 'bone')
    # ribcage
    c.rect(11, 28, 18, 11, 'bone_d'); c.rect(19, 28, 2, 11, 'bone')
    for y in (30, 33, 36): c.hline(12, y, 16, 'outline')
    # skull
    c.ellipse(20, 14, 12, 10.5, 'bone'); c.ellipse(24, 16, 8, 8, 'bone_d'); c.ellipse(19, 14, 11, 9.5, 'bone')
    j = 0 if f == 0 else 3
    c.rect(11, 22, 18, 4 + j // 2, 'bone'); c.rect(12, 26 + j, 16, 2, 'bone_d')
    for x in range(12, 29, 3): c.vline(x, 24 + j // 2, 2 + j // 2, 'outline')
    c.ellipse(14, 14, 3.5, 3.5, 'outline'); c.ellipse(26, 14, 3.5, 3.5, 'outline')
    eye = 'rune' if f == 0 else 'white'
    c.rect(13, 13, 2, 2, eye); c.rect(25, 13, 2, 2, eye)
    c.px(20, 18, 'outline'); c.px(19, 19, 'outline'); c.px(21, 19, 'outline')
    # crown
    c.rect(9, 3, 22, 3, 'gold'); c.hline(9, 3, 22, 'fire_l')
    for x in (10, 16, 20, 24, 30):
        for k in range(4): c.px(x, 2 - k, 'gold'); 
        c.px(x, -1, 'gold')
    for x in (16, 24): c.px(x, 4, 'red')
    c.px(20, 4, 'rune')
    c.outline(); return c
def heart():
    rows = [".ooo.ooo.", "oRRRoRRRo", "oRhRRRRRo", "oRRRRRRRo", ".oRRRRRo.", "..oRRRo..", "...oRo...", "....o...."]
    m = {'.':0,'o':'outline','R':'red','h':'white'}; c = C(9, 8)
    for y, r in enumerate(rows):
        for x, ch in enumerate(r): c.px(x, y, m[ch])
    return c

def arr(name, dims, items):
    return f'const uint8_t {name}{dims} = {{\n' + ',\n'.join('  {' + ','.join(map(str, it)) + '}' for it in items) + '\n};\n'
out = ['/* GENERATED by tools/mkassets.py - do not edit */', '#include "assets.h"', '']
out.append('const uint8_t PALETTE_RGB[32][3] = {' + ','.join('{%d,%d,%d}' % (r, g, b) for (_, r, g, b) in PAL) + '};\n')
tiles = [solid(False), solid(True), oneway(), spike(), ladder(), coin(0), coin(1), coin(2), coin(3), chest(), door(), solid(False, True, 5),
         rune(False), rune(True), gate(), torch(0), torch(1), skull()]
out.append(arr('TILE_SPR', '[TS_COUNT][TILE_SPR_BYTES]', [t.data() for t in tiles]))
out.append('const uint8_t PLAYER_SPR[2][10][2 + 12 * 16] = {\n  {\n' + ',\n'.join('    {' + ','.join(map(str, player(n).data())) + '}' for n in PFRAMES) +
           '\n  },\n  {\n' + ',\n'.join('    {' + ','.join(map(str, player(n).flip().data())) + '}' for n in PFRAMES) + '\n  }\n};\n')
out.append('const uint8_t CRAWLER_SPR[2][2][2 + 14 * 10] = {\n  {\n' + ',\n'.join('    {' + ','.join(map(str, crawler(f).data())) + '}' for f in (0, 1)) +
           '\n  },\n  {\n' + ',\n'.join('    {' + ','.join(map(str, crawler(f).flip().data())) + '}' for f in (0, 1)) + '\n  }\n};\n')
out.append('const uint8_t BONE_SPR[2 + 6 * 12] = {' + ','.join(map(str, bone().data())) + '};\n')
out.append(arr('KING_SPR', '[2][2 + 40 * 40]', [king(0).data(), king(1).data()]))
out.append('const uint8_t HEART_SPR[2 + 9 * 8] = {' + ','.join(map(str, heart().data())) + '};\n')
open('src/assets.c', 'w').write('\n'.join(out))
n = len(tiles) * 258 + 20 * 194 + 4 * 142 + 14 + 2 * 1602 + 74
print(f'assets ok: ~{n} bytes of sprite data, {len(PAL)} colours')
