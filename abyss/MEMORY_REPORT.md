# Memory report

**Host x86-64 `sizeof` and generated data sizes — NOT a link-map measurement of the calculator build.** eZ80 `int` is 24-bit so struct sizes differ.

```
memory (HOST sizeof - not a measurement of the calculator build)
  sizeof Game=680 Player=24 LevelState=440 SaveData=100  save file=107 bytes
  level data: 4980 bytes for 5 levels (const, not RAM); worst case 20x960 = 28800

```

| Item | Bytes | Where |
|---|---|---|
| `Game` (player, level state, 6 crawlers, 6 bones, crumble timers) | ~680 | RAM |
| `SaveData` | ~100 | RAM |
| Save file | 107 | archive, written on level end / quit / purchase only |
| Level data, 5 levels (1 byte/tile) | 4,980 | const |
| Sprites (tiles, 20 player frames, creatures, boss, UI) | ~12,400 | const |
| Framebuffer | 0 | graphx double-buffers in VRAM |

Not yet measured: code size, total RAM at runtime, final compressed `.8xp` size.
