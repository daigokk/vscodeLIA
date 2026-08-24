#pragma once
#include "Psd.h"
#include <pocketfft_hdronly.h>

#include <vector>
#include <complex>
#include <cmath>
#include <chrono>
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
        struct ComplexVector {
            std::vector<double> xs, ys;
        };
        struct Offsets {
            std::vector<std::complex<float>> chs;
            std::vector<float> phases_deg;
            bool flag = false;
        };
        struct Excitation {
            float frequency = EXCITATION_FREQUENCY;
            float amplitude = EXCITATION_AMPLITUDE;
        };
        struct Trigger {
            bool flag = false;
            bool readyFlag = false;
            bool countFlag = false;
            int nofm = 0;
            double level = 0.0;
        };
    public:
        bool pauseFlag = false;
        double dt = 0;
        int idxWrite = 0, idxCurrent = 0, nofm = 0;
        int ch_multi = 0;
        std::vector<double> times, scheduleTime;
        std::vector<ComplexVector> chs;
        std::vector<double> matrix;
        Offsets offsets;
        Excitation excitation;
        Psd psd;
        Trigger trigger;

        void init(const double rawSize, const double rawDt, const double newDt, const int ringbuffer_size, const int n_daq_channel, const int n_multiplexer_channel){
            dt = newDt;
            idxWrite = 0; idxCurrent = 0; nofm = 0;
            times.resize(ringbuffer_size);
            scheduleTime.resize(ringbuffer_size);
            for (int i = 0; i < scheduleTime.size(); ++i){
                scheduleTime[i] = i * dt * n_multiplexer_channel;
            }
            chs.resize(n_daq_channel * n_multiplexer_channel);
            for(int i=0; i < chs.size(); ++i){
                chs[i].xs.resize(ringbuffer_size, 0);
                chs[i].ys.resize(ringbuffer_size, 0);
            }
            matrix.resize(n_daq_channel * n_multiplexer_channel * ringbuffer_size);
            offsets.chs.resize(n_daq_channel * n_multiplexer_channel, 0);
            offsets.phases_deg.resize(n_daq_channel * n_multiplexer_channel, 0);
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
            // トリガー処理
            if (trigger.flag) {
                if (trigger.level >= 0) {
                    // slope:+
                    if (!trigger.readyFlag && !trigger.countFlag){
                        bool flag = true;
                        for(int ch = 0; ch < chs.size(); ++ch){
                            if(chs[ch].ys[idxCurrent] > trigger.level) {
                                flag = false;
                            }
                        }
                        if(flag){ trigger.readyFlag = true; }
                    }
                    if (trigger.readyFlag && !trigger.countFlag){
                        bool flag = false;
                        for(int ch = 0; ch < chs.size(); ++ch){
                            if(chs[ch].ys[idxCurrent] >= trigger.level) {
                                flag = true;
                                break;
                            }
                        }
                        if(flag){ trigger.countFlag = true; }
                    }   
                }
                else {
                    // slope: -
                    if (!trigger.readyFlag && !trigger.countFlag){
                        bool flag = true;
                        for(int ch = 0; ch < chs.size(); ++ch){
                            if(chs[ch].ys[idxCurrent] < trigger.level) {
                                flag = false;
                            }
                        }
                        if(flag){ trigger.readyFlag = true; }
                    }
                    if (trigger.readyFlag && !trigger.countFlag){
                        bool flag = false;
                        for(int ch = 0; ch < chs.size(); ++ch){
                            if(chs[ch].ys[idxCurrent] <= trigger.level) {
                                flag = true;
                                break;
                            }
                        }
                        if(flag){ trigger.countFlag = true; }
                    }
                }
                // Post trigger
                if(trigger.readyFlag && trigger.countFlag && trigger.countFlag){
                    if (trigger.nofm == 0) {
                        trigger.nofm = nofm;
                    }
                    else if (trigger.nofm + times.size() / 2 <= nofm) {
                        pauseFlag = true;
                        trigger.nofm = 0;
                        trigger.readyFlag = false;
                        trigger.countFlag = false;
                    }
                } 
            }
            else {
                trigger.nofm = 0;
            }
            idxCurrent = idxWrite;
            idxWrite++; nofm++;
            if(idxWrite >= times.size()) {idxWrite = 0;}
        }

        void update(const std::vector<std::vector<double>>& rawChs, const double rawDt){
            if(psd.frequency != excitation.frequency){
                psd.init(rawChs[0].size(), excitation.frequency, rawDt);
            }
            static double xs[N_DAQ_CHANNEL*N_MULTIPLEXER_CHANNEL];
            static double ys[N_DAQ_CHANNEL*N_MULTIPLEXER_CHANNEL];
            for(int i = 0; i < N_DAQ_CHANNEL; ++i){
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

    void init(
        const double rawRate=RAW_RATE, const int rawSize=RAW_SIZE,
        const int nDaqChannel=N_DAQ_CHANNEL, const int nMultiplexerChannel=N_MULTIPLEXER_CHANNEL,
        const double ringBufferDt=RINGBUFFER_DT, const int ringBufferSize=RINGBUFFER_SIZE
    ){
        rawData.init(1.0 / rawRate, rawSize, nDaqChannel * nMultiplexerChannel);
        ringBuffer.init(rawData.times.size(), rawData.rawDt, ringBufferDt, ringBufferSize, nDaqChannel, nMultiplexerChannel);
        fftBuffer.numHarmonics_x.resize(N_HARMONICS);
        fftBuffer.numHarmonics_y.resize(N_HARMONICS);
    }

    void buttonPause(){
        ringBuffer.pauseFlag = true;
    }

    void buttonRun(){
        ringBuffer.pauseFlag = false;
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
            for(int i = 0; i < size; ++i){
                int idx = (ringBuffer.idxCurrent + i) % ringBuffer.times.size();
                outFile << std::format("{:e}", ringBuffer.times[i]);
                for(int ch = 0; ch < ringBuffer.chs.size(); ++ch){
                    outFile << std::format("{0}{1:e}{0}{2:e}", delimiter, ringBuffer.chs[ch].xs[idx], ringBuffer.chs[ch].ys[idx]);
                }
                outFile << std::endl;
            }
            outFile.close();
        }
    }
};

inline void fft(Config* pCfg) {
    // TODO: ここにフーリエ変換のコードを入力
    
}
