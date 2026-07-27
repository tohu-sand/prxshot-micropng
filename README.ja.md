# prxshot-micropng

**Languages:** [English](README.md) | [日本語](README.ja.md)

BMPではなく **PNG** で保存するPSP用スクリーンショットプラグインです。

**codestation** 氏の [prxshot](https://github.com/codestation/prxshot) のフォークです。
ゲームを一時停止せずに撮影できる（オンライン中でも使える）点と、ゲームごとの
タイトルとアイコンを使ってフォルダ分けする点は、原作のまま変わっていません。

変更したのは保存形式だけです。480x272の1枚が固定383KBのBMPから約60KBになり、
可逆圧縮なので画質の劣化はありません。

| | prxshot | prxshot-micropng |
|---|---|---|
| 保存形式 | BMP | PNG（可逆） |
| 1枚のサイズ | 383KB | 約60KB |
| プラグインのサイズ | 17KB | 21KB |

## PRXShot-png との関係

**yuh0q223** 氏の **PRXShot-png** が先駆けです。prxshotをPNG対応させた
日本語圏で広く知られたフォークで、本フォークが存在するきっかけでもあります。
配布元のサイトは現在閉鎖されており、[GameBrew](https://www.gamebrew.org/wiki/PRXShot-png_PSP)でv1.1が入手できます。

PRXShot-pngはlibpngとzlibをリンクしており、
これはPC上では自然な選択ですが、プラグインは約243KBになります。
また、XMBでは撮れるのにゲーム中は撮影に失敗し、初回は破損したファイルが残り以降は
無反応になる、という報告が見られます。

代わりにPNGの書き出しを自前で実装し、実際に動く場所の制約に合わせています。
原作のprxshotからの変更は最小限で、撮影処理・ゲームID別フォルダ・PSCM.DATの
扱いには手を入れず、ファイル形式だけを変えています。

## エンコーダについて

スクリーンショットプラグインはカーネルモードのスタック4KBのスレッドで動きます。
libpngとzlibはスタックを数KB、作業メモリを約300KB要求するため、この環境には
収まりません。`micropng` はその制約に合わせて書いたPNGライタです。

- libpng・zlib・浮動小数点・再帰・`malloc` のいずれも使わない
- deflate（固定ハフマン＋LZ77）とPNGフィルタを直接実装
- スタック消費は実測で最大約300バイト（スレッドの持ち分は4KB）
- 全状態を呼び出し側が確保した1つのブロックに置き、大きなものをスタックに載せない

圧縮の前に、1回の高速なパスで画面をメモリへ退避します。表示バッファから
直接読みながら圧縮すると、その間もゲームが描画を続けるため画面が乱れます。

`micropng.c` と `micropng.h` は自己完結した移植可能なコードで、PC上でもビルド
して動かせます。`test/host_test.c` は生のRGBデータをPNGに変換するので、
任意のPNGデコーダで結果を検証できます。

## インストール

次のファイルを `ms0:/seplugins/`（プラグインを置いている場所）にコピーします。

- `prxshot.prx` — プラグイン本体
- `prxshot.ini` — 設定ファイル
- `default_icon0.png` — アイコンを持たないhomebrew用の代替アイコン
- `xmb.sfo` — VSHモードでのフォルダアイコン用テンプレート

そのうえで `game.txt`（XMBでも撮りたい場合は `vsh.txt` にも）へ追記します。

```
ms0:/seplugins/prxshot.prx 1
```

ARK-4などでは代わりに `PLUGINS.TXT` へ追記します。

```
game, ms0:/SEPLUGINS/prxshot.prx, on
```

**NOTE（♪）ボタン**で撮影します。保存先は次のとおりです。

```
ms0:/PSP/SCREENSHOT/<ゲームID>/pic_0000.png
```

homebrewとPBP形式のゲームはIDが共通のため、タイトルから生成した
`PS<16進8桁>` という名前のフォルダになります。

## 設定 (`prxshot.ini`)

`[General]`

| キー | 既定値 | 内容 |
|---|---|---|
| `CreatePic1` | `0` | `1` にするとゲームの背景画像もスクリーンショットフォルダに保存する |
| `PSPGoUseMS0` | `0` | PSP Goで内蔵ストレージではなくM2カードに保存したい場合は `1` |
| `XMBClearCache` | `0` | `1` にすると撮影後にXMBのキャッシュを更新する。新しい画像が表示されない問題は解消するが、ゲームカテゴリー使用時にフリーズすることがある |
| `ScreenshotKey` | `0x800000` | 撮影に使うボタン（複数の組み合わせも可） |
| `KeyTimeout` | `0` | ボタンを押してから撮影するまでの遅延（ミリ秒） |
| `ScreenshotName` | `%s/pic_%04d.png` | ファイル名の書式。`%s/`・`%04d`・拡張子はこの順で必須 |

`[CustomKeys]` と `[CustomTimeout]` では、ゲームIDごとにボタンや遅延を
上書きできます。

```
ULJM05800 = 0x000009   # SELECT + START
```

ボタンの値:

| ボタン | 値 | ボタン | 値 |
|---|---|---|---|
| SELECT | `0x000001` | ↑ | `0x000010` |
| START | `0x000008` | → | `0x000020` |
| Lトリガー | `0x000100` | ↓ | `0x000040` |
| Rトリガー | `0x000200` | ← | `0x000080` |
| △ | `0x001000` | HOME | `0x010000` |
| ○ | `0x002000` | ♪（NOTE） | `0x800000` |
| × | `0x004000` | 画面表示 | `0x400000` |
| □ | `0x008000` | 音量+ / 音量- | `0x100000` / `0x200000` |

## ビルド方法

[pspdev](https://github.com/pspdev/pspdev) を導入したうえで実行します。

```sh
export PSPDEV=/path/to/pspdev
export PATH=$PSPDEV/bin:$PATH
make
```

エンコーダはPC上でも動かせます。変更を確認する際はこれが最も手軽です。

```sh
gcc -std=c99 -Wall -Wextra -O2 -I. -o host_test micropng.c test/host_test.c
./host_test frame.rgb 480 272 out.png
```

## 原作からの変更点

- スクリーンショットをPNGで保存（`micropng.c`, `micropng.h`, `png_write.c`, `png_write.h`）
- `ScreenshotName` の既定値を `.png` に変更
- 現行のpspdevツールチェインでビルドできるよう修正（C99の `inline` のリンケージ、
  SDKのバージョン間で移動したヘッダ、`_ctype_` テーブルが使えないカーネルlibc
  モードでの `toupper`）

## ライセンス

prxshotから継承したGPLv3です。全文は [LICENSE](LICENSE) を参照してください。

- prxshot: Copyright (C) 2011 Codestation
- PNG対応部分: Copyright (C) 2026 豆腐さんど (tohu_sand)

同梱の `minIni`（CompuPhase作）はApache License 2.0です。`minIni.c` の冒頭に
ライセンス表記があります。
