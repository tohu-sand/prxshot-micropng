# prxshot-micropng

**Languages:** [English](README.md) | [日本語](README.ja.md)

A PSP screenshot plugin that saves **PNG** instead of BMP.

This is a fork of [prxshot](https://github.com/codestation/prxshot) by
**codestation**. Everything that makes prxshot pleasant to use is unchanged: it
captures without pausing the game, so it works during online play, and it sorts
screenshots into per-game folders using each game's own title and icon.

The only functional change is the output format. A 480x272 screenshot drops
from a fixed 383 KB BMP to roughly 60 KB, and stays lossless.

| | prxshot | prxshot-micropng |
|---|---|---|
| Output | BMP | PNG (lossless) |
| Typical size | 383 KB | ~60 KB |
| Plugin size | 17 KB | 21 KB |

## Relation to PRXShot-png

**PRXShot-png** by **yuh0q223** got there first: a well known Japanese fork of
prxshot that also writes PNG, and the reason this one exists. Its original site
is gone; [GameBrew](https://www.gamebrew.org/wiki/PRXShot-png_PSP) still hosts
v1.1.

PRXShot-png links libpng and zlib, which is the natural choice on a desktop but
grows the plugin to roughly 243 KB. There are also reports of captures failing
inside games while XMB captures keep working, the first attempt leaving a
corrupt file and later attempts doing nothing.

This fork writes the PNG itself instead, sized for where the code actually
runs. It stays close to upstream prxshot: the capture path, the game-id folders
and the PSCM.DAT handling are untouched, and only the file format changes.

## The encoder

A screenshot plugin runs in kernel mode on threads with a 4 KB stack, and
libpng with zlib wants several kilobytes of stack and around 300 KB of working
memory. `micropng` is a PNG writer written for those constraints:

- No libpng, no zlib, no floating point, no recursion, no `malloc`
- Deflate (fixed Huffman with LZ77) and the PNG filters are implemented directly
- Peak stack use measured at ~300 bytes, against the 4 KB the thread has
- All state lives in one caller-owned block, so nothing large touches the stack

The frame is copied to memory in a single fast pass before compression starts.
Compressing straight off the display buffer would tear, because the game keeps
drawing while the encoder works.

`micropng.c` and `micropng.h` are self contained and portable; they build and
run on a host as well, and `test/host_test.c` encodes a raw RGB file so the
result can be checked against any PNG decoder.

## Installation

Copy these to `ms0:/seplugins/` (or wherever you keep plugins):

- `prxshot.prx` — the plugin
- `prxshot.ini` — configuration
- `default_icon0.png` — icon used for homebrew that has none
- `xmb.sfo` — template for the folder icon in VSH mode

Then add the plugin to `game.txt` (and `vsh.txt` if you want XMB screenshots):

```
ms0:/seplugins/prxshot.prx 1
```

On ARK-4 and similar, add it to `PLUGINS.TXT` instead:

```
game, ms0:/SEPLUGINS/prxshot.prx, on
```

Press the **NOTE** button to take a screenshot. Files are written to:

```
ms0:/PSP/SCREENSHOT/<GAME_ID>/pic_0000.png
```

Homebrew and PBP games share a game id, so they get a folder named
`PS<8 hex digits>` derived from the title instead.

## Configuration (`prxshot.ini`)

`[General]`

| Key | Default | Meaning |
|---|---|---|
| `CreatePic1` | `0` | Set to `1` to also store the game's background image in the screenshot folder |
| `PSPGoUseMS0` | `0` | Set to `1` on a PSP Go to save to the M2 card instead of internal storage |
| `XMBClearCache` | `0` | Set to `1` to refresh the XMB cache after a shot. Fixes new screenshots not appearing, but can freeze with Game Categories |
| `ScreenshotKey` | `0x800000` | Button, or a combination, that triggers a capture |
| `KeyTimeout` | `0` | Delay in milliseconds between the key press and the capture |
| `ScreenshotName` | `%s/pic_%04d.png` | Filename pattern. `%s/`, `%04d` and the extension are required, in that order |

`[CustomKeys]` and `[CustomTimeout]` take a game id and override the key or the
timeout for that game only:

```
ULJM05800 = 0x000009   # SELECT + START
```

Button values:

| Button | Value | Button | Value |
|---|---|---|---|
| SELECT | `0x000001` | UP | `0x000010` |
| START | `0x000008` | RIGHT | `0x000020` |
| LTRIGGER | `0x000100` | DOWN | `0x000040` |
| RTRIGGER | `0x000200` | LEFT | `0x000080` |
| TRIANGLE | `0x001000` | HOME | `0x010000` |
| CIRCLE | `0x002000` | NOTE | `0x800000` |
| CROSS | `0x004000` | SCREEN | `0x400000` |
| SQUARE | `0x008000` | VOLUP / VOLDOWN | `0x100000` / `0x200000` |

## Building

With the [pspdev](https://github.com/pspdev/pspdev) toolchain installed:

```sh
export PSPDEV=/path/to/pspdev
export PATH=$PSPDEV/bin:$PATH
make
```

The encoder can also be exercised on a host, which is the quickest way to
check a change:

```sh
gcc -std=c99 -Wall -Wextra -O2 -I. -o host_test micropng.c test/host_test.c
./host_test frame.rgb 480 272 out.png
```

## Changes from upstream

- Screenshots are written as PNG (`micropng.c`, `micropng.h`, `png_write.c`, `png_write.h`)
- `ScreenshotName` defaults to `.png`
- Builds with current pspdev toolchains: C99 `inline` linkage, headers that moved
  between SDK versions, and `toupper` in kernel libc mode where the `_ctype_`
  table is unavailable

## License

GPLv3, inherited from prxshot. See [LICENSE](LICENSE) for the full text.

- prxshot: Copyright (C) 2011 Codestation
- PNG support: Copyright (C) 2026 豆腐さんど (tohu_sand)

`minIni` by CompuPhase is bundled under the Apache License 2.0; see the header
of `minIni.c`.
