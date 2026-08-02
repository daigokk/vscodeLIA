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
  - [Digilent Waveforms](https://digilent.com/reference/software/waveforms/waveforms-3/start)
  - [WaveForms-SDK-Getting-Started-Cpp](https://github.com/Digilent/WaveForms-SDK-Getting-Started-Cpp)
  - [GLFW](https://www.glfw.org/)
  - [Dear ImGui](https://github.com/ocornut/imgui) & [ImPlot](https://github.com/epezent/implot)
  - [pocketfft](https://github.com/mreineck/pocketfft)

## 開発環境のセットアップ(OSはWindowsを想定)
  1. Waveforms
     1. 以下からダウンロードしてインストール
          ```
          https://digilent.com/reference/software/waveforms/waveforms-3/previous-versions
          ```
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
     1. 拡張機能
        - C/C++ for Visual Studio Code
        - C/C++ DevTools
        - C/C++ Extension Pack
        - C/C++ Themes
        - Makefile Tools
  1. (必要に応じて)git
      1. [git](https://git-scm.com/install/windows)をインストールし、vscodeを再起動
      1. 「ようこそ」タブの「Gitリポジトリのクローン...」をクリック
      1. vscode上部の「command center」に「Githubから複製」が表示されるので以下のURLを指定
          ```
          https://github.com/daigokk/vscodeLIA/
          ```
      1. 任意のフォルダを選択し「リポジトリの宛先として選択」をクリック
      1. 「リポジトリを開きますか、または現在のワークスペースに追加しますか？」と表示されるので「開く」をクリック

## 開発方法
  1. 本ページ上部の「<>Code▽」から「Download ZIP」をクリック
  1. 任意のフォルダに解凍する。
  1. 「main.cpp」を開き、vscode左のツールバーの一番上ファイルのアイコン「エクスプローラー」をクリックし、現れる「フォルダーを開く」ボタンを押し、現在のフォルダを選択する。
  1. しばらく待つか「F5」を押すと、左のツールバーの一番下に「C++」の記載がある「Makefile」アイコンが現れるのでクリック
     - 構成: [Default]
     - ターゲットのビルド: [all]
     - 起動ターゲット: [vscodeLIA.exe]
     - Makefile: [./Makefile]
     - Make: [mingw32-make.exe]
  1. 「MAKEFILE」ウィンドウ上の右三角ボタンでビルドされた実行ファイル(exe)を実行。実行ファイルが存在しなけれがビルドを行う。
  1. 虫ボタンでデバックが可能(ブレイクポイントが有効、件数の値を確認可能、等)
  1. 「MAKEFILE」ウィンドウ上の右三角ボタンでビルドされた実行ファイル(exe)を実行。実行ファイルが存在しなけれがビルドを行う。
  1. 虫ボタンでデバックが可能(ブレイクポイントが有効、件数の値を確認可能、等)
