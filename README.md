# vscodeLIA

## 開発環境
  - [Visual Studio Code](https://code.visualstudio.com/download)
  - [Mingw-w64](https://github.com/niXman/mingw-builds-binaries/releases/)

## 依存ソフトウェア:
  - [WaveForms-SDK-Getting-Started-Cpp](https://github.com/Digilent/WaveForms-SDK-Getting-Started-Cpp)
  - [GLFW](https://www.glfw.org/)
  - [Dear ImGui](https://github.com/ocornut/imgui) & [ImPlot](https://github.com/epezent/implot)
  - [pocketfft](https://github.com/mreineck/pocketfft)

## 開発環境のセットアップ
  1. Mingw-w64をダウンロード
    https://github.com/niXman/mingw-builds-binaries/releases/x86_64-16.1.0-release-win32-seh-ucrt-rt_v14-rev1.7z
  1. 以下へコピー
     C:\Program Files\mingw64
  1. Pathを通す
     - スタートメニュー → 設定 → システム → バージョン情報 の右側にある 「システムの詳細設定」 をクリック
     - 「環境変数(N)…」ボタンを押す
     - 「システム環境変数」の一覧から Path を選択し、編集(E) → 新規(N) で以下を入力
       - C:\Program Files\mingw64\bin
     - 再起動または再度ログイン
