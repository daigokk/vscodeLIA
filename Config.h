#pragma once
#include "Psd.h"
#include <pocketfft_hdronly.h>

#include <vector>
#include <cmath>
#include <chrono>

#define RAW_COUNT 10000
#define RAW_RATE 100e6
#define EXCITATION_FREQUENCY 100e3
#define EXCITATION_AMPLITUDE 1.0
#define RINGBUFFER_DT 2e-3 // 2ms
#define RINGBUFFER_SIZE 10000
#define N_DAQ_CHANNEL 2
#define N_MULTIPLEXER_CHANNEL 1
#define N_HARMONICS 5


// 測定に関する設定および測定値を保存するクラス
class Config{
public:
    const double PI_ = std::acos(-1.0);
    int ch_multi = 0;
    Psd psd;
    class Excitation {
    public:
        float frequency = EXCITATION_FREQUENCY;
        float amplitude = EXCITATION_AMPLITUDE;
    } excitation;

    class Status {
    public:
        bool isRun = false;
        std::string deviceSerial = "No connected";
    } status;

    class RawData {
    public:
        double dt = 1.0 / RAW_RATE;
        std::vector<double> times;
        std::vector<std::vector<double>> chs;
    } rawData;

    class ComplexData {
    public:
        std::vector<double> xs, ys;
    };
    
    class RingBuffer {
    public:
        double dt = RINGBUFFER_DT;
        std::vector<double> times, scheduleTime;
        std::vector<ComplexData> chs;
        std::vector<double> matrix;
        int idxWrite = 0, idxCurrent = 0;
        void init(const int n_channel = N_DAQ_CHANNEL * N_MULTIPLEXER_CHANNEL){
            times.resize(RINGBUFFER_SIZE);
            scheduleTime.resize(RINGBUFFER_SIZE);
            for (int i = 0; i < scheduleTime.size(); ++i){
                scheduleTime[i] = i * dt * n_channel;
            }
            chs.resize(n_channel);
            matrix.resize(n_channel * RINGBUFFER_SIZE);
            for(int i=0; i < chs.size(); ++i){
                chs[i].xs.resize(RINGBUFFER_SIZE, 0);
                chs[i].ys.resize(RINGBUFFER_SIZE, 0);
            }
        }
        void pop(const double xs[], const double ys[]){
            static std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
            times[idxWrite] = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - start_time
            ).count();
            for(int ch = 0; ch < chs.size(); ++ch){
                chs[ch].xs[idxWrite] = xs[ch];
                chs[ch].ys[idxWrite] = ys[ch];
                const int idx = ch * times.size() + idxWrite;
                matrix[idx] = ys[ch];
            }
            idxCurrent = idxWrite;
            idxWrite++;
            if(idxWrite >= times.size()) {idxWrite = 0;}
        }
    } ringBuffer;

    class FFTBuffer {
    public:
        std::vector<double> numHarmonics_x, numHarmonics_y;
    } fftBuffer;
    
    void RawInit(const int raw_count, const int n_channel = N_DAQ_CHANNEL * N_MULTIPLEXER_CHANNEL) {
        rawData.times.resize(raw_count);
        rawData.chs.resize(n_channel);
        for(int i=0; i < rawData.chs.size(); i++){
            rawData.chs[i].resize(raw_count);
        }
        double wdt = 2.0 * PI_ * excitation.frequency * rawData.dt;
        for (int i = 0; i < rawData.times.size(); i++) {
            rawData.times[i] = static_cast<double>(i) * rawData.dt;
        }
    }
    
    void update(){
        static double t = 0;
        if(psd.frequency != excitation.frequency){
            psd.init(rawData.times.size(), excitation.frequency, rawData.dt);
        }
        static double xs[N_DAQ_CHANNEL*N_MULTIPLEXER_CHANNEL];
        static double ys[N_DAQ_CHANNEL*N_MULTIPLEXER_CHANNEL];
        for(int i = 0; i < ringBuffer.chs.size() / N_MULTIPLEXER_CHANNEL; ++i){
            const int ch = i + ch_multi * N_DAQ_CHANNEL;
            auto const [x, y] = psd.calc(rawData.chs[ch].data());
            xs[ch] = x;
            ys[ch] = y;
        }
        ch_multi++;
        if(ch_multi >= N_MULTIPLEXER_CHANNEL){
            ringBuffer.pop(xs, ys);
            ch_multi = 0;
        }
    }

    explicit Config() {
        RawInit(RAW_COUNT);
        ringBuffer.init();
        fftBuffer.numHarmonics_x.resize(N_HARMONICS);
        fftBuffer.numHarmonics_y.resize(N_HARMONICS);
        psd.init(rawData.times.size(), excitation.frequency, rawData.dt);
    }
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

inline void fft(Config* pCfg) {
    // TODO: ここにフーリエ変換のコードを入力
    
}
