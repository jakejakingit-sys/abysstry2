# ABYSS — TI-84 CE side-scrolling dungeon game (work in progress)

**World 1 (Forgotten Crypt) is playable end-to-end: 5 levels + the Bone King. Worlds 2–4 (15 levels, 3 guardians) are not built yet.**

## Controls
| Key | Action |
|---|---|
| ← → | Move |
| 2ND | Jump (hold = higher; coyote time + jump buffer) |
| ↑ / ↓ | Climb ladders; ↓ drops through one-way ledges |
| ALPHA | **Rune Pulse** — lights ancient runes within 4 tiles (costs 20 energy, 0.5 s cooldown) |
| ENTER / CLEAR | Confirm / back, pause in a level |

## What's in World 1
1. **The Entrance** – spikes, pit, stairs, ladder to a secret chest, a patrolling bone-crawler.
2. **Forgotten Halls** – a spike-floor trial crossed on one-way ledges; hidden chest up a ladder.
3. **Burial Grounds** – collapsing floors over pits, a sealed tomb opened by a rune (chest inside), secret ledge.
4. **Bone Chambers** – moving platforms over a pit and a spike field, two runes (one up a ladder) that open the exit gate.
5. **Bone King's Throne** – guardian fight: bones rain from the ceiling (telegraphed by a red flash), faster per rune lit. Climb the ledges, light all three runes, the Bone King collapses, the gate opens.

Complete screen after each level, level select with all 20 slots (world panels, done/locked/soon markers), upgrades (Armor / Max Health / Energy, 5 levels each, rising prices), Collection (abilities, treasure records, completion %), Stats. Progress saved to the archive (`ABYSSSV`) on level end, quit, and purchase.

## Verification status — read this
* **Host tests (real, run with gcc): `make -f Makefile.host`** → 4,823 checks, 0 failures. Covers physics, collision, platforms, crumbling, runes/gates, creatures, boss logic, upgrades, saves (every bit-flip detected), level validity and a simulated-input check that the spike field, the pit and the moving-platform gap are crossable.
* **CEdev compile:** the front end (`src/main.c`) is built by CI on the real toolchain; this version adds `gfx_Sprite` and `timer_*` calls that have not been compiled yet.
* **Not verified:** actual play on a calculator/CEmu by the author, frame rate, and the 8xp size. Level *reachability* is a heuristic plus the simulated-input checks, not a full solver — if a jump feels impossible, tell me which level and where.

## Not implemented
Worlds 2–4; Heavy Landing / Phase Dash / Parry / Ground Wave / Whirlwind / Resonance gameplay (they unlock by level but do nothing yet); sound; dash/crouch/hurt/death/victory animations; relics; the final ending; world-specific tilesets for worlds 2–4.

## Build
`make -f Makefile.host` (tests; regenerates level + asset C from `levels/` and `tools/`), `make` (CEdev → `bin/ABYSS.8xp`).
Edit levels in `levels/world1/*.txt` (legend in `tools/author_world1.py`) — no extra tools needed beyond Python 3.
