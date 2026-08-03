# vscodeLIA (Template of Simple Software Lock-in Amplifier)

![Hard copy](./docs/images/HardCopy.png)

---

## 概要

**vscodeLIA** は、DAQ (Data Acquisition) を利用して動作する、C++製のシンプルなソフトウェア・ロックインアンプ(LIA)です。

* **主な機能:**
    * Digilent製DAQ(Analog Discovery)を用いた信号の測定・集録
    * ImPlotを用いた波形のリアルタイム描画
    * 位相敏感検波(同期検波)等による信号処理(未実装)
      * **※注意:** 学習を目的の一つとしているため、位相敏感検波処理は**あえて未実装**にしています。


* **このリポジトリの発展形:**
より実用的なフル機能のソフトウェア・ロックインアンプをお探しの場合は、[LIA (daigokk/LIA)](https://github.com/daigokk/LIA/) をご参照ください。
* **開発目的:**
データ集録・GUI描画・信号処理を組み合わせた計測アプリの学習用サンプルコード、または新規計測プロジェクトの雛形（テンプレート）としての使用を想定しています。

---

## 開発環境・技術スタック

* **エディタ / IDE:** [Visual Studio Code](https://code.visualstudio.com/download)
* **コンパイラ:** [Mingw-w64](https://github.com/niXman/mingw-builds-binaries/releases/) (GCC for Windows)
* **対象OS:** Windows 10 / 11 (64-bit)

### 依存ライブラリ・SDK

本プロジェクトでは以下のサードパーティ製ライブラリ・SDKを利用しています。

| ライブラリ / SDK | 概要 / 用途 |
| --- | --- |
| **[Digilent WaveForms SDK](https://digilent.com/reference/software/waveforms/waveforms-3/start)** | Analog Discovery等のDigilentハードウェア制御用SDK |
| **[GLFW](https://www.glfw.org/)** | OpenGLウィンドウ作成および入力処理 |
| **[Dear ImGui](https://github.com/ocornut/imgui)** | 軽量グラフィカルユーザーインターフェース (GUI) |
| **[ImPlot](https://github.com/epezent/implot)** | Dear ImGui向けのリアルタイムプロット・グラフ描画拡張 |
| **[pocketfft](https://github.com/mreineck/pocketfft)** | ヘッダーオンリーのFFT (高速フーリエ変換) ライブラリ |

---

## 開発環境のセットアップ (Windows)

以下のステップ順にセットアップを行ってください。

### 1. Digilent WaveForms のインストール

計測用ドライバおよびSDKを取得するためにインストールします。

1. [WaveForms 過去バージョン / ダウンロードページ](https://digilent.com/reference/software/waveforms/waveforms-3/previous-versions) にアクセスします。
1. Windows用のインストーラー（例: `digilent.waveforms_vX.X.X_64bit.exe`）をダウンロードして実行します。
1. インストール時のコンポーネント選択で **WaveForms SDK** が選択されていることを確認して完了させます。

### 2. Mingw-w64 (C++コンパイラ) の配置とパス設定

1. 以下のリンクから Mingw-w64 のアーカイブをダウンロードします。
  ```
  https://github.com/niXman/mingw-builds-binaries/releases/x86_64-16.1.0-release-win32-seh-ucrt-rt_v14-rev1.7z
  ```


1. ダウンロードした `.7z` ファイルを解凍し、中身の `mingw64` フォルダを以下のディレクトリへ移動（コピー）します。
```text
C:\Program Files\mingw64

```


*(※ `C:\Program Files\mingw64\bin\g++.exe` が存在する構造になるように配置してください)*
1. **環境変数 (PATH) の設定:**
* キーボードの `Win + R` を押し、`sysdm.cpl` と入力して Enter（または 「スタートメニュー」→「設定」→「システム」→「バージョン情報」→「システムの詳細設定」）。
* **「環境変数(N)…」** ボタンをクリック。
* 「システム環境変数」欄の一覧から **`Path`** を選択し、**「編集(E)…」** をクリック。
* **「新規(N)」** を押し、以下を入力して OK を押します。
```text
C:\Program Files\mingw64\bin

```




1. **動作確認:**
コマンドプロンプトを開き、以下を実行してバージョンが表示されれば設定完了です。
```cmd
g++ --version

```



### 3. Visual Studio Code のセットアップ

1. [VS Code 公式サイト](https://code.visualstudio.com/download) からインストーラーを取得し、インストールします。
1. VS Codeを起動し、左側の拡張機能タブ（`Ctrl + Shift + X`）を開き、以下の拡張機能を検索してインストールします。
* **C/C++** (`ms-vscode.cpptools`)
* **C/C++ Extension Pack** (`ms-vscode.cpptools-extension-pack`)
* **Makefile Tools** (`ms-vscode.makefile-tools`)



---

## プロジェクトの取得方法

いずれかの方法でソースコードを取得してください。

### 方法A: ZIPダウンロードを使用する場合

1. リポジトリページ上部の **「<> Code」** ボタン → **「Download ZIP」** をクリックします。
2. ダウンロードしたZIPファイルを任意の場所に解凍します。
3. VS Codeのメニューから **「ファイル」→「フォルダーを開く...」** を選択し、解凍したフォルダを開きます。

---

### 方法B: Gitを使用する場合

1. [Git for Windows](https://www.google.com/search?q=https://git-scm.com/install/windows) をインストールします。
2. VS Codeを起動し、上部検索バー（Command Center）または `Ctrl + Shift + P` でコマンドパレットを開きます。
3. `Git: Clone`（Git: クローン）と入力・選択し、以下のURLを入力します。
```text
https://github.com/daigokk/vscodeLIA.git

```


4. 保存先のフォルダを選択し、クローン完了後に表示されるダイアログで **「開く」** を選択します。


## ビルドおよび実行方法

* 本プロジェクトは VS Code の **Makefile Tools** 拡張機能を利用してビルド・デバッグを行うように構成されています。
* makeを用いることで、ビルド時間が大幅に短縮されます。

1. **Makefile Toolsの初回認識:**
フォルダを開いた後、左側サイドバーに **C/C++のアイコンがついた「Makefile Tools」**（または `MAKEFILE` タブ）が表示されます。
*※表示されない場合は、`main.cpp` などを開くか、`F5` キーを一回押すとアクティブ化されます。*


2. **Makefile設定の確認:**
Makefile Tools のパネル内で、以下のように設定されているか確認・指定します。

* **構成 (Configuration):** `Default`
* **ターゲットのビルド (Build target):** `all`
* **起動ターゲット (Launch target):** `vscodeLIA.exe`
* **Makefile:** `./Makefile`
* **Make メイク:** `mingw32-make.exe` (または `make`)


3. **ビルドと実行:**
* **実行 (Run):** Makefile Tools パネルの上部にある **再生ボタン（右三角 ▶）** をクリックします。自動的に `mingw32-make` が呼び出されてコンパイル・ビルドが実行され、完了後に `vscodeLIA.exe` が起動します。
* **デバッグ実行 (Debug):** **虫アイコン** をクリックします。ブレイクポイントの設定、ステップ実行、変数レジスタの監視を行いながらデバッグが可能です。


---

## 補足

* **Visual Studio (MSVC) との比較:**
Visual Studio（IDE）を使用すると Makefile やデバッガの手動設定なしで開発を始められますが、商業利用時の有償化の制限があります。本プロジェクトは、オープンソースかつ軽量な **VS Code + GCC (MinGW-w64)** の組み合わせで開発できるように設計されています。VS Codeでももっと簡単にC++開発ができるかもしれません。
* **WaveForms SDKのリンクエラーが発生する場合:**
WaveFormsがデフォルトのパス（`C:\Program Files (x86)\Digilent\WaveFormsSDK` 等）にインストールされているか、`Makefile` 内のライブラリインクルードパス・リンクパスを確認してください。
