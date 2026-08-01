# vscodeLIA
  ![Hard copy](./docs/images/HardCopy.png)

## 概要
  - シンプルなソフトウェアロックインアンプ
    - 測定、波形表示、位相敏感検波(あえて未実装)
    - シンプルではないソフトウェアロックインアンプは[こちら](https://github.com/daigokk/LIA/)
  - 学習やひな形としての使用を想定しています。

## 開発環境
  - [Visual Studio Code](https://code.visualstudio.com/download)
  - [Mingw-w64](https://github.com/niXman/mingw-builds-binaries/releases/)

## 依存ソフトウェア:
  - [WaveForms-SDK-Getting-Started-Cpp](https://github.com/Digilent/WaveForms-SDK-Getting-Started-Cpp)
  - [GLFW](https://www.glfw.org/)
  - [Dear ImGui](https://github.com/ocornut/imgui) & [ImPlot](https://github.com/epezent/implot)
  - [pocketfft](https://github.com/mreineck/pocketfft)

## 開発環境のセットアップ(OSはWindowsを想定)
  1. Mingw-w64
     1. 以下をダウンロード
          ```
          https://github.com/niXman/mingw-builds-binaries/releases/x86_64-16.1.0-release-win32-seh-ucrt-rt_v14-rev1.7z
          ```
     1. 解凍して以下へコピー
         ```
         C:\Program Files\mingw64
         ```
     1. Pathを通す
         - スタートメニュー → 設定 → システム → バージョン情報 の右側にある 「システムの詳細設定」 をクリック
         - 「環境変数(N)…」ボタンを押す
         - 「システム環境変数」の一覧から Path を選択し、編集(E) → 新規(N) で以下を入力
             ```
             C:\Program Files\mingw64\bin
             ```
       - 再起動または再度ログイン
  1. Visual Studio Code
     1. Visual Studio Codeのインストール
     2. 拡張機能
        - C/C++ for Visual Studio Code
        - C/C++ DevTools
        - C/C++ Extension Pack
        - C/C++ Themes
        - Makefile Tools

## 開発方法
  1. vscode左のツールバーの「Makefile」に設定
     - 構成: [Default]
     - ターゲットのビルド: [all]
     - 起動ターゲット: [vscodeLIA.exe]
     - Makefile: [./Makefile]
     - Make: mingw32-make.exe
  1. 「MAKEFILE」ウィンドウ上の右三角ボタンでビルドされた実行ファイル(exe)を実行。実行ファイルが存在しなけれがビルドを行う。
  3. 虫ボタンでデバックが可能(ブレイクポイントが有効、件数の値を確認可能、等)
