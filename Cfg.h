#pragma once
#include <vector>
#include <cmath>
#define RAW_COUNT 10000
#define RAW_RATE 100e6
#define RING_BUFFER_SIZE 1000
#define EXCITATION_FREQUENCY 100e3
#define EXCITATION_AMPLITUDE 1.0
#define BUFFER_DT 2e-3 // 2ms
#define PI acos(-1.0)


class Cfg{
public:
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
        double x, y;
    };
    
    class Buffer {
    public:
        double dt = BUFFER_DT;
        ComplexData ch1, ch2;
    } buffer;
    
    Cfg() {
        rawData.times.resize(RAW_COUNT);
        rawData.ch1.resize(RAW_COUNT);
        rawData.ch2.resize(RAW_COUNT);
        double wdt = 2.0 * PI * excitation.frequency * rawData.dt;
        for (int i = 0; i < RAW_COUNT; i++) {
            rawData.times[i] = static_cast<double>(i) * rawData.dt;
            rawData.ch1[i] = std::sin(i * wdt);
            rawData.ch2[i] = std::cos(i * wdt);
        }
    }
};