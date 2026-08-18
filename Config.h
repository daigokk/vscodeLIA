#pragma once
#include <vector>
#include <cmath>
#define RAW_COUNT 10000
#define RAW_RATE 100e6
#define EXCITATION_FREQUENCY 100e3
#define EXCITATION_AMPLITUDE 1.0
#define RINGBUFFER_DT 2e-3 // 2ms
#define RINGBUFFER_SIZE 1
#define N_DAQ_CHANNEL 2
#define N_MULTIPLEXER_CHANNEL 1
#define N_HARMONICS 5

// 測定に関する設定および測定値を保存するクラス
class Config{
public:
    const double PI_ = std::acos(-1.0);
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
        std::vector<std::vector<double>> ch;
    } rawData;

    class ComplexData {
    public:
        std::vector<double> xs, ys;
    };
    
    class RingBuffer {
    public:
        double dt = RINGBUFFER_DT;
        std::vector<double> times;
        std::vector<ComplexData> ch;
    } ringBuffer;

    class FFTBuffer {
    public:
        std::vector<double> numHarmonics_x, numHarmonics_y;
    } fftBuffer;
    
    void RawInit(const int raw_count, const int n_channel = N_DAQ_CHANNEL * N_MULTIPLEXER_CHANNEL) {
        rawData.times.resize(raw_count);
        rawData.ch.resize(n_channel);
        for(int i=0; i < rawData.ch.size(); i++){
            rawData.ch[i].resize(raw_count);
        }
        double wdt = 2.0 * PI_ * excitation.frequency * rawData.dt;
        for (int i = 0; i < rawData.times.size(); i++) {
            rawData.times[i] = static_cast<double>(i) * rawData.dt;
        }
    }

    void RingBufferInit(const int n_channel = N_DAQ_CHANNEL * N_MULTIPLEXER_CHANNEL){
        ringBuffer.times.resize(n_channel);
        ringBuffer.ch.resize(n_channel);
        for(int i=0; i < ringBuffer.ch.size(); i++){
            ringBuffer.ch[i].xs.resize(RINGBUFFER_SIZE, 0);
            ringBuffer.ch[i].ys.resize(RINGBUFFER_SIZE, 0);
        }
    }

    explicit Config() {
        RawInit(RAW_COUNT);
        RingBufferInit();
        fftBuffer.numHarmonics_x.resize(N_HARMONICS);
        fftBuffer.numHarmonics_y.resize(N_HARMONICS);
    }
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};
