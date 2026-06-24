# Shooting

DxLib を使用した Windows 向けの弾幕シューティングゲームです。複数の敵弾パターンをステージとして収録し、ステージ選択、リプレイ再生、ベストタイム保存に対応しています。

## 概要

- 画面サイズは 640 x 480 のウィンドウモードです。
- ステージごとに異なる弾幕パターンと BGM を読み込みます。
- クリア時のベストタイムを `saveData/bestTime.json` に保存します。
- クリア時の入力履歴をリプレイデータとして保存し、メニューから再生できます。
- ウィンドウ位置・サイズ、カーソル位置などの設定を `saveData` 配下に保存します。

## 必要環境

- Windows
- Visual Studio 2022 以降（Platform Toolset v143）
- C++17 対応コンパイラ
- [DxLib](https://dxlib.xsrv.jp/)
- nlohmann/json

> 現在の Visual Studio プロジェクトでは、追加インクルードディレクトリとして `D:\tools\DxLib_VC\include` と `D:\tools\nlohmann`、追加ライブラリディレクトリとして `D:\tools\DxLib_VC\include` が設定されています。環境に合わせて `Shooting/Shooting.vcxproj` のパスを変更してください。

## セットアップ

1. DxLib と nlohmann/json を用意します。
2. 必要に応じて `Shooting/Shooting.vcxproj` のインクルードパス・ライブラリパスを自分の環境に合わせて修正します。
3. `Shooting.sln` を Visual Studio で開きます。
4. `Debug|Win32`、`Release|Win32`、`Debug|x64`、`Release|x64` のいずれかを選択してビルドします。
5. 実行時に `assets` と `saveData` が実行ディレクトリから参照できるように配置してください。

## 操作方法

### メニュー

| キー | 操作 |
| --- | --- |
| テンキー 4 / 6 / 8 / 5 | ステージカーソルの左右上下移動 |
| テンキー 7 / 9 | ページ切り替え |
| V | 選択したステージを開始 |
| R | 選択したステージのリプレイを再生 |
| Q | 終了 |

### ゲーム中

| キー | 操作 |
| --- | --- |
| テンキー 4 / 6 / 8 / 5 | 自機の左右上下移動 |
| C | 低速移動 |
| V | ショット |
| Q | メニューへ戻る |
| B | デバッグ用の無敵 |

### クリア / ミス後

| キー | 操作 |
| --- | --- |
| V | リトライ |
| R | リプレイ再生中にリプレイを再開 |
| N | 次のステージへ進む |
| Q | メニューへ戻る |

## ディレクトリ構成

```text
.
├── Shooting.sln                  # Visual Studio ソリューション
├── README.md                     # このファイル
└── Shooting/
    ├── Shooting.vcxproj          # Visual Studio プロジェクト
    ├── main.cpp                  # エントリーポイント、ゲームループ
    ├── stageData.cpp             # ステージ定義
    ├── enemyPat_*.cpp            # 敵弾パターン実装
    ├── imgSoundLoad.cpp          # 画像・効果音・BGM 読み込み
    ├── replay.cpp                # リプレイ処理
    ├── fileOpenClose.cpp         # セーブデータの読み書き
    └── saveData/                 # ベストタイム、リプレイ、設定ファイル
```

## アセット

ゲームは以下のようなアセットを `assets` 配下から読み込みます。

- `assets/images/` : プレイヤー、敵、敵弾、スプラッシュ画像
- `assets/sounds/` : 効果音
- `assets/bgm/` : メニューおよびステージ BGM

アセットが見つからない場合、画像や音声が正しく読み込まれないため、実行ディレクトリから相対パスで参照できるようにしてください。

## セーブデータ

`Shooting/saveData` 配下に以下のファイルを保存・参照します。

- `bestTime.json` : ステージごとのベストタイム
- `cursor.json` : メニューカーソル位置
- `window.json` : ウィンドウ位置・サイズ
- `replay_*.dat` : ステージごとのリプレイデータ

## 開発メモ

- 新しいステージを追加する場合は、弾幕パターン関数を実装し、`stageData.cpp` の `stageData` にステージ ID、説明、BGM 名、関数ポインタを追加します。
- 画像・効果音・BGM を追加する場合は、`imgSoundLoad.cpp` の読み込み処理と `assets` 配下のファイル配置をそろえてください。
- リプレイは移動、低速、ショットなどのキー入力履歴を保存して再現します。
