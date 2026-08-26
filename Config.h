#pragma once
#include "RingBuffer.h"
#include <pocketfft_hdronly.h>

#include <vector>
#include <format>
#include <fstream>

#define RAW_SIZE 10000
#define RAW_RATE 100e6
#define EXCITATION_FREQUENCY 10e3
#define EXCITATION_AMPLITUDE 1.0
#define N_DAQ_CHANNEL 2
#define N_MULTIPLEXER_CHANNEL 8
#define N_HARMONICS 5
#define RINGBUFFER_DT 2e-3 // 2ms
#define RINGBUFFER_SIZE (10 / RINGBUFFER_DT / N_MULTIPLEXER_CHANNEL) // 10s


// 測定に関する設定および測定値を保存するクラス
class Config{
public:
    const double PI_ = std::acos(-1.0);
    
    struct Status {
        bool isRun = false;
        std::string deviceSerial = "No connected";
    } status;

    class RawData {
    public:
        double range = 2.5;
        double rawDt = 0;
        std::vector<double> times;
        std::vector<std::vector<double>> chs;
        void init(const double newDt, const int raw_count, const int n_channel) {
            rawDt = newDt;
            times.resize(raw_count);
            chs.resize(n_channel);
            for(int i = 0; i < chs.size(); ++i){
                chs[i].resize(raw_count);
            }
            for (int i = 0; i < times.size(); ++i) {
                times[i] = static_cast<double>(i) * rawDt;
            }
        }
    } rawData;

    RingBuffer ringBuffer;

    class FFTBuffer {
    public:
        std::vector<double> numHarmonics_x, numHarmonics_y;
    } fftBuffer;

    void init(
        const double rawRate=RAW_RATE, const int rawSize=RAW_SIZE,
        const int nDaqChannel=N_DAQ_CHANNEL, const int nMultiplexerChannel=N_MULTIPLEXER_CHANNEL,
        const double ringBufferDt=RINGBUFFER_DT, const int ringBufferSize=RINGBUFFER_SIZE
    ){
        rawData.init(1.0 / rawRate, rawSize, nDaqChannel * nMultiplexerChannel);
        ringBuffer.initSource(EXCITATION_FREQUENCY, 1, 0);
        ringBuffer.init(ringBufferDt, ringBufferSize, nDaqChannel, nMultiplexerChannel);
        fftBuffer.numHarmonics_x.resize(N_HARMONICS);
        fftBuffer.numHarmonics_y.resize(N_HARMONICS);
    }

    void buttonPause(){
        ringBuffer.pauseFlag = true;
    }

    void buttonRun(){
        ringBuffer.ch_multi = 0;
        ringBuffer.idxCurrent = 0;
        ringBuffer.idxWrite = 0;
        ringBuffer.nofm = 0;
        for(int i = 0; i< ringBuffer.times.size(); ++i){
            for(int ch = 0; ch < ringBuffer.chs.size(); ++ch){
                ringBuffer.chs[ch].xs[i] = 0.0;
                ringBuffer.chs[ch].ys[i] = 0.0;
                int idx = ch * ringBuffer.times.size() + i;
                ringBuffer.matrix[idx] = 0.0;
            }
        }
        ringBuffer.pauseFlag = false;
    }

    explicit Config() {
        init();
    }
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    ~Config(){
        std::ofstream outFile("ect.csv");
        if (outFile.is_open()) {
            const char delimiter = ',';
            // ファイルヘッダー
            outFile << "t(s)";
            for(int ch = 0; ch < ringBuffer.chs.size(); ++ch){
                outFile << std::format("{0}ch{1}x{0}ch{1}y", delimiter, ch+1);
            }
            outFile << std::endl;
            // 測定値
            const int size = ringBuffer.times.size() < ringBuffer.nofm ? ringBuffer.times.size() : ringBuffer.nofm;
            int startIdx = 0;
            if(ringBuffer.nofm > ringBuffer.times.size()){
                startIdx = ringBuffer.idxWrite;
            }
            for(int i = 0; i < size; ++i){
                int idx = (startIdx + i) % ringBuffer.times.size();
                outFile << std::format("{:e}", ringBuffer.times[idx]);
                for(int ch = 0; ch < ringBuffer.chs.size(); ++ch){
                    outFile << std::format("{0}{1:e}{0}{2:e}", delimiter, ringBuffer.chs[ch].xs[idx], ringBuffer.chs[ch].ys[idx]);
                }
                outFile << std::endl;
            }
            outFile.close();
        }
    }
};


inline void fft(Config& cfg) {
    const auto& in_data = cfg.rawData.chs[0];
    const size_t N = in_data.size();
    const size_t N_HARMONICS_ = cfg.fftBuffer.numHarmonics_x.size();
    // 周波数分解能 df = 1 / (N * dt)
    const double df = 1.0 / (static_cast<double>(N) * cfg.rawData.rawDt);
    const double f0 = cfg.ringBuffer.sourceChs[0].frequency;
    // 正規化用係数（DFT結果を平均振幅に戻すため 2/N を乗算）
    const double scale = 2.0 / static_cast<double>(N);

    // TODO: ここにフーリエ変換のコードを入力

    // 1. pocketfft実行用の入出力形状およびストライドの設定
    pocketfft::shape_t shape = { N };
    pocketfft::stride_t stride_in = { sizeof(double) };
    pocketfft::stride_t stride_out = { sizeof(std::complex<double>) };
    pocketfft::shape_t axes = { 0 };

    // Real-to-Complex FFT (r2c) の出力サイズは N/2 + 1
    std::vector<std::complex<double>> fft_out(N / 2 + 1);

    // 2. FFTの実行 (r2c: 実数入力 -> 複素数出力)
    // 引数: shape, stride_in, stride_out, axes, forward(true), in_ptr, out_ptr, scale(1.0)
    pocketfft::r2c(
        shape,
        stride_in,
        stride_out,
        axes,
        pocketfft::FORWARD,
        in_data.data(),
        fft_out.data(),
        1.0
    );
    // ここまで

    // 3. 各倍波に対応する周波数インデックス（ビン）を特定して格納
    for (int i = 0; i < N_HARMONICS; ++i) {
        // 抽出対象の高調波倍率 (1倍, 3倍, 5倍, ...)
        const double target_freq = f0 * (i * 2 + 1);
        
        // 最も近い周波数ビンのインデックスを計算
        const size_t bin_idx = static_cast<size_t>(std::round(target_freq / df));

        // ナイキスト周波数（N/2）以下の範囲内にあるか確認
        if (bin_idx < fft_out.size()) {
            // スケーリングを適用して実部(X)と虚部(Y)を格納
            cfg.fftBuffer.numHarmonics_x[i] = fft_out[bin_idx].real() * scale;
            cfg.fftBuffer.numHarmonics_y[i] = fft_out[bin_idx].imag() * scale;
        }
    }
}
