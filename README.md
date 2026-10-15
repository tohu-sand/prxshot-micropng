# prxshot-micropng

[English](README.md) | [日本語](README.ja.md)

A PSP screenshot plugin based on [prxshot](https://github.com/codestation/prxshot)
by codestation. It captures the screen without pausing the game and saves
480×272 screenshots as lossless PNG files, organized into folders with each
game's title and icon.

PNG support was inspired by PRXShot-png by yuh0q223.

## Installation

Copy these files to the same directory, such as `ms0:/seplugins/`:

- `prxshot-micropng.prx` — plugin
- `prxshot-micropng.ini` — configuration
- `default_icon0.png` — fallback folder icon
- `xmb.sfo` — folder metadata for XMB screenshots

Add the following line to `game.txt`. To capture the XMB, add it to `vsh.txt`
as well:

```text
ms0:/seplugins/prxshot-micropng.prx 1
```

For ARK-4, enable the plugin in games through `PLUGINS.TXT`:

```text
game, ms0:/seplugins/prxshot-micropng.prx, on
```

If you install the plugin elsewhere, adjust the path accordingly.

## Usage

Press the **NOTE (♪)** button to take a screenshot. The default output path is:

```text
ms0:/PSP/SCREENSHOT/<GAME_ID>/pic_0000.png
```

PSP Go uses `ef0:/PSP/SCREENSHOT/` unless `PSPGoUseMS0` is enabled.
XMB screenshots go in the `XMB` folder. Homebrew and PBP games use folder names
derived from their titles: `PS<8 hex digits>`.

## Configuration

Edit `prxshot-micropng.ini` beside the plugin. These settings belong in
`[General]`:

| Key | Default | Meaning |
|---|---|---|
| `PSPGoUseMS0` | `0` | Set to `1` on a PSP Go to save to the M2 card instead of internal storage |
| `XMBClearCache` | `0` | Set to `1` to refresh the XMB cache after a screenshot; may conflict with Game Categories |
| `ScreenshotKey` | `0x800000` | Button or button combination used to take a screenshot |
| `KeyTimeout` | `0` | How long the buttons must be held before capture, in milliseconds |
| `ScreenshotName` | `%s/pic_%04d.png` | Filename pattern; keep `%s/`, `%04d` and `.png` in this order |

`CreatePic1` is read separately from `[General]` in `prxshot.ini` beside the
plugin, not from `prxshot-micropng.ini`. It defaults to `1`, which includes the
game's background image in the folder metadata. Set it to `0` to omit that image.

Use `[CustomKeys]` and `[CustomTimeout]` to override the button combination
and hold time for a game ID:

```ini
[CustomKeys]
ULJM05800 = 0x000009 # SELECT + START

[CustomTimeout]
ULJM05800 = 1000
```

Add the button values together to form a combination:

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

Install the [pspdev](https://github.com/pspdev/pspdev) toolchain, then run:

```sh
export PSPDEV=/path/to/pspdev
export PATH="$PSPDEV/bin:$PATH"
make
```

The `micropng` encoder has no libpng or zlib dependency and can also be built
on a host. The following example converts a 480×272 raw RGB image to PNG.
The input must contain three bytes per pixel, in R, G, B order:

```sh
gcc -std=c99 -Wall -Wextra -O2 -I. -o host_test micropng.c test/host_test.c
./host_test frame.rgb 480 272 out.png
```

## License

GPLv3 or later, inherited from prxshot. See [LICENSE](LICENSE).

- prxshot: Copyright (C) 2011 Codestation
- PNG support: Copyright (C) 2026 豆腐さんど (tohu_sand)

`minIni` by CompuPhase is bundled under the Apache License 2.0; see the header
of `minIni.c`.
