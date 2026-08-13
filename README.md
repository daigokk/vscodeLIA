# vscodeLIA (Template of Simple Software Lock-in Amplifier)


| ![Hard copy](./docs/images/HardCopy.png) |
| --- |
| 図1. Hard copy |
---

## 1. 概要

図1に示す**vscodeLIA** は、DAQ (Data Acquisition) を利用して動作する、C++製のシンプルなソフトウェア・ロックインアンプ(LIA)です。

* **このプロジェクトの目的:**
データ集録・GUI描画・信号処理を組み合わせた、以下のような使用を想定しています。
  * **計測アプリの学習用サンプルコード**
  * **新規計測プロジェクトの雛形 (テンプレート)**
  * [職業能力開発総合大学校](https://www.uitec.jeed.go.jp/) 電気工学専攻 3年時プレゼミ([応用センシング研究室](https://www.uitec.jeed.go.jp/kenkyu/laboratory/lab039/index.html))の課題

* **主な機能:**
    * Digilent製DAQ([Analog Discovery](https://digilent.com/shop/analog-discovery-3/))を用いた信号の測定・集録
      * `Daq.h`、`Daq.cpp`及び`Makefile`を書き換えることで、他のDAQ(例えばNI-DAQ)に対応させることも可能です。
    * [ImPlot](https://github.com/epezent/implot)を用いた波形のリアルタイム描画
    * [位相敏感検波](https://www.youtube.com/watch?v=pHyuB1YW4qY)(同期検波)等による信号処理(図2参照、未実装)
      * **※注意:** 学習を目的の一つとしているため、位相敏感検波処理は**あえて未実装**にしています。
    * オープンソースである **[Visual Studio Code](https://code.visualstudio.com/) + GCC ([MinGW-w64](https://www.mingw-w64.org/))** の組み合わせで開発できるように設計されています。

* **このプロジェクトの発展形:**
より実用的なフル機能のソフトウェア・ロックインアンプをお探しの場合は、[LIA (daigokk/LIA)](https://github.com/daigokk/LIA/) をご参照ください。





| ![Phase sensitive detection](./docs/images/PSD.svg) |
| --- |
| 図2. 位相敏感検波のブロック図 |

* [参考] 以下のコードは図2のブロック図を具現化したものです。`Daq.cpp`に記載の`psd`関数に以下を記述すると、プローブの状態に合わせてリアルタイムにXYウィンドウの輝点が移動します。

```c++
void psd(Cfg* pCfg){
    double ch1x = 0, ch1y = 0;
    for (size_t i = 0; i < pCfg->rawData.ch1.size(); ++i) {
        double wt = 2 * pCfg->PI_ * pCfg->excitation.frequency * pCfg->rawData.dt * i;
        ch1x += pCfg->rawData.ch1[i] * 2 * sin(wt);
        ch1y += pCfg->rawData.ch1[i] * 2 * cos(wt);
    }
    pCfg->buffer.ch1.xs[0] = ch1x / pCfg->rawData.ch1.size();
    pCfg->buffer.ch1.ys[0] = ch1y / pCfg->rawData.ch1.size();
}
```

---

## 2. ハードウェア

* 図3、図4、及び表1は、自己誘導差動型の渦電流プローブのブリッジ・プリアンプの回路と必要な部品を示しています。ご自身のアプリケーションに合わせて設計・製作してください。

| ![Schematic](./docs/images/Schematic.svg) ![Schematic_AD620](./docs/images/AD620.svg) |
| --- |
| 図3. 回路図 |

| ![Front of circuit board](./docs/images/CircuitBoard_front.jpg) | ![Back of circuit board](./docs/images/CircuitBoard_back.jpg) |
| --- | --- |
| (a) 表 | (b) 裏 |
| 図4. 電子回路基板 |  |

表1. 部品一覧
  | 部品 | 型番 | 備考 |
  | ---- | ---- | ---- |
  | DAQ | Digilent Analog Discovery 3 | [Analog Discovery 3: 125 MS/s USB Oscilloscope, Waveform Generator, Logic Analyzer, and Variable Power Supply](https://digilent.com/reference/test-and-measurement/analog-discovery-3/start) |
  | L型ピンソケット | 2×15 | https://akizukidenshi.com/catalog/g/g113419/ |
  | ブレッドボード |  47×36mm  | https://akizukidenshi.com/catalog/g/g111960/ |
  | 計装アンプ | Analog Devices AD620ANZ | https://akizukidenshi.com/catalog/g/g113693/ |
  | ゲイン設定用抵抗(40dB) | 510Ω | [See "Gain Selection" on page 15 of the AD620 datasheet.](https://www.analog.com/media/en/technical-documentation/data-sheets/AD620.pdf) |
  | コンデンサ | 0.1uF×2 | https://akizukidenshi.com/catalog/g/g110149/ |
  | (表面実装コンデンサ) | 0.1uF×2 | https://akizukidenshi.com/catalog/g/g116143/ |
  | 可変抵抗器 | 100Ω | https://akizukidenshi.com/catalog/g/g117821/ |
  | 同軸ケーブル | 特性インピーダンス50Ω | https://akizukidenshi.com/catalog/g/g116943/|
  | $L_1$, Sensor coil| 励磁周波数で50Ω程度 | 例えば https://akizukidenshi.com/catalog/g/g116967/ |
  | $L_2$, Reference coil | 励磁周波数で50Ω程度 | 例えば https://akizukidenshi.com/catalog/g/g116967/ |
  | (必要であれば)メスコネクタ | 多治見無線電機 PRC03-12A10-7F10.5 | 探傷器側コネクタ |
---

## 3. 開発環境のセットアップ (Windows)

本プロジェクトでは開発環境として以下を使用します。

* **DAQ:** [Digilent Analog Discovery 3](https://digilent.com/shop/analog-discovery-3/)
* **コンパイラ:** [Mingw-w64](https://github.com/niXman/mingw-builds-binaries/releases/) (GCC for Windows)
* **エディタ / IDE:** [Visual Studio Code](https://code.visualstudio.com/)
* **対象OS:** Windows 10 / 11 (64-bit)

以下を参考にセットアップを行ってください。

### ⚡ Digilent WaveForms のインストール

DAQのドライバおよびSDKを取得するためにインストールします。

1. [WaveForms 過去バージョン / ダウンロードページ](https://digilent.com/reference/software/waveforms/waveforms-3/previous-versions) にアクセスします。
1. Windows用のインストーラー（例: `digilent.waveforms_vX.X.X_64bit.exe`）をダウンロードして実行します。
1. インストール時の設定はデフォルトでよいです。

### 🔄 Mingw-w64 (C++コンパイラ) の配置とパス設定

1. [niXman/mingw-builds-binaries](https://github.com/niXman/mingw-builds-binaries/releases/)にアクセスします。
1. Mingw-w64 のアーカイブ(例: `x86_64-16.1.0-release-win32-seh-ucrt-rt_v14-rev1.7z`)をダウンロードします。
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
  コマンドプロンプトを開き、以下を実行してバージョンが表示されれば設定完了です。表示されない場合、コマンドプロンプトを再起動する、または一度ログアウトすると、環境変数の設定が更新されるのでうまくいくかもしれません。
    ```cmd
    g++ --version
    ```


### 📝 Visual Studio Code のセットアップ

1. [VS Code 公式サイト](https://code.visualstudio.com/download) からインストーラーを取得し、インストールします。
1. VS Codeを起動し、左側の拡張機能タブ（`Ctrl + Shift + X`）を開き、以下の拡張機能を検索してインストールします。
   * **C/C++** (`ms-vscode.cpptools`)
   * **C/C++ Extension Pack** (`ms-vscode.cpptools-extension-pack`)
   * **Makefile Tools** (`ms-vscode.makefile-tools`)


---

## 4. プロジェクトの取得方法

* いずれかの方法でソースコードを取得してください。

### 方法A: ZIPダウンロードを使用する場合

1. リポジトリページ上部の **「<> Code」** ボタン → **「Download ZIP」** をクリックします。
1. ダウンロードしたZIPファイルを任意の場所に解凍します。
1. VS Codeのメニューから **「ファイル」→「フォルダーを開く...」** を選択し、解凍したフォルダを開きます。


### 方法B: Gitを使用する場合

1. [Git for Windows](https://www.google.com/search?q=https://git-scm.com/install/windows) をインストールします。
1. VS Codeを起動し、上部検索バー（Command Center）または `Ctrl + Shift + P` でコマンドパレットを開きます。
1. `Git: Clone`（Git: クローン）と入力・選択し、以下のURLを入力します。
   ```text
   https://github.com/daigokk/vscodeLIA.git
   ```
1. 保存先のフォルダを選択し、クローン完了後に表示されるダイアログで **「開く」** を選択します。


* このプロジェクトは表2に示すライブラリに依存しています。偉大なる先人に感謝！なお、以下のライブラリはプロジェクトに含まれているため新たにダウンロードする必要はありません。

表2. 依存ライブラリ一覧
| ライブラリ | 概要 / 用途 |
| --- | --- |
| **[Digilent WaveForms SDK](https://digilent.com/reference/software/waveforms/waveforms-sdk/reference-manual)** | Analog Discovery等のDigilentハードウェア制御用SDK |
| **[Digilent/WaveForms-SDK-Getting-Started-Cpp](https://github.com/Digilent/WaveForms-SDK-Getting-Started-Cpp)** | 上記SDKのラッパー。このラッパーを参考に`Daq.h`を作成。 |
| **[GLFW](https://www.glfw.org/)** | OpenGLウィンドウ作成および入力処理 |
| **[Dear ImGui](https://github.com/ocornut/imgui)** | 軽量グラフィカルユーザーインターフェース (GUI) |
| **[ImPlot](https://github.com/epezent/implot)** | Dear ImGui向けのリアルタイムプロット・グラフ描画拡張 |
| **[pocketfft](https://github.com/mreineck/pocketfft)** | ヘッダーオンリーのFFT (高速フーリエ変換) ライブラリ |

---


## 5. ビルドおよび実行方法

* 本プロジェクトは VS Code の **Makefile Tools** 拡張機能を利用してビルド・デバッグを行うように構成されています。
* makeを用いることで、修正していない`cpp`ファイルのビルドが除外されるので、ビルド時間が大幅に短縮されます。

1. **Makefile Toolsの初回認識:**
フォルダを開いた後、左側サイドバーに **C/C++のアイコンがついた「Makefile Tools」**（または `MAKEFILE` タブ）が表示されます。
*※表示されない場合は、左側サイドバーを右クリックし、`Makefile`にチェックをつけてください。*


1. **Makefile設定の確認:**
Makefile Tools のパネル内で、以下のように設定します。

    * **構成 (Configuration):** `Default`
    * **ターゲットのビルド (Build target):** `all`
    * **起動ターゲット (Launch target):** `vscodeLIA.exe`
    * **Makefile Path:** `Makefile`
    * **Make Path:** `mingw32-make.exe`

1. **ビルドと実行:**
    * **実行 (Run):** Makefile Tools パネルの上部にある **再生ボタン（右三角 ▶）** をクリックします。自動的に `mingw32-make` が呼び出されてコンパイル・ビルドが実行され、完了後に `vscodeLIA.exe` が起動します。
    * **デバッグ実行 (Debug):** **虫アイコン** をクリックします。ブレイクポイントの設定、ステップ実行、変数レジスタの監視を行いながらデバッグが可能です。


1. **補足:**

    * **[Visual Studio](https://visualstudio.microsoft.com/) (Microsoft Visual C++, MSVC) との比較:** MSVCを使用すると Makefile やデバッガの手動設定なしで開発を始められます。商業利用時は有償ではありますが、MSVCはとても素晴らしいツールです。本プロジェクトは、多少不便ではありますがオープンソースである **VS Code + GCC (MinGW-w64)** の組み合わせで開発できるように設計されています。VS Codeでももっと簡単にC++開発ができるかもしれません。
    * **WaveForms SDKのリンクエラーが発生する場合:** WaveFormsがデフォルトのパス（`C:\Program Files (x86)\Digilent\WaveFormsSDK` 等）にインストールされているか、`Makefile` 内のライブラリインクルードパス・リンクパスを確認してください。


---
## 6. 課題

* `Daq.cpp`に記載の`psd`関数を完成させてください。
* (オプション) `fft`関数を完成させ、FFTを使って同様の結果を得られることを確認してみてください。様々な条件においてどちらが優れているか比較してみるのもよいでしょう。
* (オプション) この実装は、2msごとに 10000[Sample]/100[MSample/s]=0.1[ms] 分だけAD変換しています。2msはUSBの制限から決定しました。この方式の利点はPSDの実装が簡単(平均を使える、FFTを使える)であること、100MS/sの高速なAD変換ができること、等が挙げられます。しかしながら全体の時間の 0.1[ms]/[2ms]=5[%] しか使っていません。検出信号の95%は捨てていることを意味します。言い換えると2msの間プローブの検出信号が一定の場合は、問題ないです。この制限(95%を捨てる)は、AD変換の速度を落とすことで使えるようになる、DAQのストリーミング記録(100%使う)を用いることで解決できます。AD変換の速度を落とすことは検出周波数の最大値が下がることを意味しますが、AD変換の前段にスーパーヘテロダインを用いることでその制限も回避することができます。何を言っているのかわかったでしょうか？ご理解いただけたら、ハードウェアおよびソフトウェアを改造して、信号の取りこぼしのない、かつ100kHzの検出信号を検波できるLIAを実装してみてください。
* (オプション) 追加したい機能はありませんか？その機能を実装してみましょう。[LIA (daigokk/LIA)](https://github.com/daigokk/LIA/) が参考になるかもしれません。
* (オプション) C++からメモリ安全なRustに書き換えてみましょう。

---
