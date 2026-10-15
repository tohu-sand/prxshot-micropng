# prxshot-micropng

[English](README.md) | [日本語](README.ja.md)

codestation氏の[prxshot](https://github.com/codestation/prxshot)を基にしたPSP用スクリーンショットプラグインです。ゲームを一時停止せずに撮影し、480×272の画面をPNG形式（可逆圧縮）で保存します。画像はゲームのタイトルとアイコンを使ったフォルダにまとめます。

PNG対応のきっかけは、yuh0q223氏のPRXShot-pngです。

## インストール

次のファイルを同じディレクトリ（例：`ms0:/seplugins/`）にコピーします。

- `prxshot-micropng.prx`：プラグイン本体
- `prxshot-micropng.ini`：設定ファイル
- `default_icon0.png`：フォルダ用の代替アイコン
- `xmb.sfo`：XMBのスクリーンショットフォルダ用メタデータ

`game.txt`に次の行を追加します。XMBでも撮影する場合は`vsh.txt`にも追加します。

```text
ms0:/seplugins/prxshot-micropng.prx 1
```

ARK-4では、`PLUGINS.TXT`に次の行を追加して、ゲーム中にプラグインが動作するように設定します。

```text
game, ms0:/seplugins/prxshot-micropng.prx, on
```

別の場所にインストールした場合は、その場所に合わせてパスを書き換えてください。

## 使い方

**NOTE（♪）ボタン**で撮影します。既定の保存先は次のとおりです。

```text
ms0:/PSP/SCREENSHOT/<ゲームID>/pic_0000.png
```

PSP Goの保存先は`ef0:/PSP/SCREENSHOT/`です。`PSPGoUseMS0`を有効にするとM2カードに保存します。XMBのスクリーンショットは`XMB`フォルダに保存します。homebrewとPBP形式のゲームでは、タイトルから生成した`PS<16進8桁>`という名前のフォルダに保存します。

## 設定

プラグインと同じディレクトリの`prxshot-micropng.ini`を編集します。次の設定は`[General]`に記述します。

| キー | 既定値 | 内容 |
|---|---|---|
| `PSPGoUseMS0` | `0` | PSP Goで内蔵ストレージではなくM2カードに保存する場合は`1` |
| `XMBClearCache` | `0` | `1`にすると撮影後にXMBのキャッシュを更新する。Game Categoriesと競合する場合がある |
| `ScreenshotKey` | `0x800000` | 撮影に使うボタン、またはボタンの組み合わせ |
| `KeyTimeout` | `0` | 撮影するためにボタンを押し続ける時間（ミリ秒） |
| `ScreenshotName` | `%s/pic_%04d.png` | ファイル名の書式。`%s/`・`%04d`・`.png`をこの順に含める |

`CreatePic1`は`prxshot-micropng.ini`ではなく、プラグインと同じディレクトリの`prxshot.ini`の`[General]`から読み込まれます。省略時は`1`となり、フォルダのメタデータにゲームの背景画像を含めます。不要な場合は`0`を指定します。

`[CustomKeys]`と`[CustomTimeout]`では、ゲームIDごとにボタンの組み合わせと長押し時間を指定できます。

```ini
[CustomKeys]
ULJM05800 = 0x000009 # SELECT + START

[CustomTimeout]
ULJM05800 = 1000
```

複数のボタンを組み合わせる場合は、値を足し合わせます。

| ボタン | 値 | ボタン | 値 |
|---|---|---|---|
| SELECT | `0x000001` | ↑ | `0x000010` |
| START | `0x000008` | → | `0x000020` |
| Lトリガー | `0x000100` | ↓ | `0x000040` |
| Rトリガー | `0x000200` | ← | `0x000080` |
| △ | `0x001000` | HOME | `0x010000` |
| ○ | `0x002000` | ♪（NOTE） | `0x800000` |
| × | `0x004000` | 画面表示 | `0x400000` |
| □ | `0x008000` | 音量+/音量- | `0x100000` / `0x200000` |

## ビルド

[pspdev](https://github.com/pspdev/pspdev)を導入し、次のコマンドを実行します。

```sh
export PSPDEV=/path/to/pspdev
export PATH="$PSPDEV/bin:$PATH"
make
```

`micropng`エンコーダはlibpngとzlibに依存せず、PC上でもビルドできます。次の例では、480×272ピクセルのRGBデータをPNGに変換します。入力ファイルには、各ピクセルのR・G・Bの値を1バイトずつ、この順に並べます。ヘッダーは不要です。

```sh
gcc -std=c99 -Wall -Wextra -O2 -I. -o host_test micropng.c test/host_test.c
./host_test frame.rgb 480 272 out.png
```

## ライセンス

prxshotと同じく、GPL（バージョン3またはそれ以降）に基づいて配布しています。ライセンスの全文は[LICENSE](LICENSE)を参照してください。

- prxshot: Copyright (C) 2011 Codestation
- PNG対応部分: Copyright (C) 2026 豆腐さんど (tohu_sand)

同梱の`minIni`（CompuPhase作）にはApache License 2.0が適用されます。`minIni.c`の冒頭にライセンス表記があります。
