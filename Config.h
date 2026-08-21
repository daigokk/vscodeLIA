#pragma once
#include "Psd.h"
#include <pocketfft_hdronly.h>

#include <vector>
#include <complex>
#include <cmath>
#include <chrono>

#define RAW_COUNT 10000
#define RAW_RATE 100e6
#define EXCITATION_FREQUENCY 10e3
#define EXCITATION_AMPLITUDE 1.0
#define RINGBUFFER_DT 2e-3 // 2ms
#define RINGBUFFER_SIZE 1000
#define N_DAQ_CHANNEL 2
#define N_MULTIPLEXER_CHANNEL 8
#define N_HARMONICS 5


// 測定に関する設定および測定値を保存するクラス
class Config{
public:
    const double PI_ = std::acos(-1.0);
    
    class Status {
    public:
        bool isRun = false;
        std::string deviceSerial = "No connected";
    } status;

    class RawData {
    public:
        double rawDt = 0;
        std::vector<double> times;
        std::vector<std::vector<double>> chs;
        void init(const double newDt, const int raw_count, const int n_channel) {
            rawDt = newDt;
            times.resize(raw_count);
            chs.resize(n_channel);
            for(int i=0; i < chs.size(); i++){
                chs[i].resize(raw_count);
            }
            for (int i = 0; i < times.size(); i++) {
                times[i] = static_cast<double>(i) * rawDt;
            }
        }
    } rawData;

    class RingBuffer {
    private:
        class ComplexVector {
        public:
            std::vector<double> xs, ys;
        };
        class Offsets {
        public:
            std::vector<std::complex<float>> chs;
            std::vector<float> phases_deg;
            bool flag = false;
        };
        class Excitation {
        public:
            float frequency = EXCITATION_FREQUENCY;
            float amplitude = EXCITATION_AMPLITUDE;
        };
    public:
        double dt = 0;
        int idxWrite = 0, idxCurrent = 0;
        int ch_multi = 0;
        std::vector<double> times, scheduleTime;
        std::vector<ComplexVector> chs;
        std::vector<double> matrix;
        Offsets offsets;
        Excitation excitation;
        Psd psd;

        void init(const double rawSize, const double rawDt, const double newDt, const int ringbuffer_size, const int n_channel){
            dt = newDt;
            idxWrite = 0; idxCurrent = 0;
            times.resize(ringbuffer_size);
            scheduleTime.resize(ringbuffer_size);
            for (int i = 0; i < scheduleTime.size(); ++i){
                scheduleTime[i] = i * dt * n_channel;
            }
            chs.resize(n_channel);
            for(int i=0; i < chs.size(); ++i){
                chs[i].xs.resize(ringbuffer_size, 0);
                chs[i].ys.resize(ringbuffer_size, 0);
            }
            matrix.resize(n_channel * ringbuffer_size);
            offsets.chs.resize(n_channel, 0);
            offsets.phases_deg.resize(n_channel, 0);
            psd.init(rawSize, excitation.frequency, rawDt);
        }
        
        void pop(const double xs[], const double ys[]){
            static std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
            times[idxWrite] = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start_time
            ).count();
            if (offsets.flag){
                for(int ch = 0; ch < chs.size(); ++ch){
                    offsets.chs[ch].real(xs[ch]);
                    offsets.chs[ch].imag(ys[ch]);
                }
                offsets.flag = false;
            }
            for(int ch = 0; ch < chs.size(); ++ch){
                auto [x, y] = psd.rotate(
                    offsets.phases_deg[ch],
                    xs[ch] - offsets.chs[ch].real(),
                    ys[ch] - offsets.chs[ch].imag()
                );
                chs[ch].xs[idxWrite] = x;
                chs[ch].ys[idxWrite] = y;
                const int idx = ch * times.size() + idxWrite;
                matrix[idx] = chs[ch].ys[idxWrite];
            }
            idxCurrent = idxWrite;
            idxWrite++;
            if(idxWrite >= times.size()) {idxWrite = 0;}
        }

        void update(const std::vector<std::vector<double>>& rawChs, const double rawDt){
            if(psd.frequency != excitation.frequency){
                psd.init(rawChs[0].size(), excitation.frequency, rawDt);
            }
            static double xs[N_DAQ_CHANNEL*N_MULTIPLEXER_CHANNEL];
            static double ys[N_DAQ_CHANNEL*N_MULTIPLEXER_CHANNEL];
            for(int i = 0; i < chs.size() / N_MULTIPLEXER_CHANNEL; ++i){
                const int ch = i + ch_multi * N_DAQ_CHANNEL;
                auto const [x, y] = psd.calc(rawChs[ch].data());
                xs[ch] = x;
                ys[ch] = y;
            }
            ch_multi++;
            if(ch_multi >= N_MULTIPLEXER_CHANNEL){
                pop(xs, ys);
                ch_multi = 0;
            }
        }
    } ringBuffer;

    class FFTBuffer {
    public:
        std::vector<double> numHarmonics_x, numHarmonics_y;
    } fftBuffer;

    void init(){
        rawData.init(1.0 / RAW_RATE, RAW_COUNT, N_DAQ_CHANNEL * N_MULTIPLEXER_CHANNEL);
        ringBuffer.init(rawData.times.size(), rawData.rawDt, RINGBUFFER_DT, RINGBUFFER_SIZE, N_DAQ_CHANNEL * N_MULTIPLEXER_CHANNEL);
        fftBuffer.numHarmonics_x.resize(N_HARMONICS);
        fftBuffer.numHarmonics_y.resize(N_HARMONICS);
    }

    explicit Config() {
        init();
    }
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

inline void fft(Config* pCfg) {
    // TODO: ここにフーリエ変換のコードを入力
    
}
