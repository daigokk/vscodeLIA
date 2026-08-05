#pragma once
#include <vector>
#include <cmath>
#define RAW_COUNT 10000
#define RAW_RATE 100e6
#define EXCITATION_FREQUENCY 100e3
#define EXCITATION_AMPLITUDE 1.0
#define BUFFER_DT 2e-3 // 2ms
#define BUFFER_SIZE 1


class Cfg{
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
        char deviceName[32] = {0};
        char serialNumber[32] = "Disconnected";
    } status;

    class RawData {
    public:
        double dt = 1.0 / RAW_RATE;
        std::vector<double> times, ch1, ch2;
    } rawData;

    class ComplexData {
    public:
        std::vector<double> xs, ys;
    };
    
    class Buffer {
    public:
        double dt = BUFFER_DT;
        ComplexData ch1, ch2;
    } buffer;

    class FFTBuffer {
    public:
        std::vector<double> numHarmonics_x, numHarmonics_y;
    } fftBuffer;
    
    Cfg() {
        rawData.times.resize(RAW_COUNT);
        rawData.ch1.resize(RAW_COUNT);
        rawData.ch2.resize(RAW_COUNT);
        double wdt = 2.0 * PI_ * excitation.frequency * rawData.dt;
        for (int i = 0; i < RAW_COUNT; i++) {
            rawData.times[i] = static_cast<double>(i) * rawData.dt;
            rawData.ch1[i] = std::sin(i * wdt);
            rawData.ch2[i] = std::cos(i * wdt);
        }
        buffer.ch1.xs.resize(BUFFER_SIZE);
        buffer.ch1.ys.resize(BUFFER_SIZE);
        buffer.ch2.xs.resize(BUFFER_SIZE);
        buffer.ch2.ys.resize(BUFFER_SIZE);
        fftBuffer.numHarmonics_x.resize(5);
        fftBuffer.numHarmonics_y.resize(5);
    }
};