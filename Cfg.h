#pragma once
#include <vector>
#include <cmath>
#define SAMPLE_COUNT 10000
#define SAMPLE_RATE 100e6
#define RING_BUFFER_SIZE 1000
#define EXCITATION_FREQUENCY 100e3
#define EXCITATION_AMPLITUDE 1.0
#define PI acos(-1.0)

class Cfg{
public:
    double dt = 1.0 / SAMPLE_RATE;

    class Excitation {
    public:
        double frequency = EXCITATION_FREQUENCY;
        double amplitude = EXCITATION_AMPLITUDE;
    } excitation;

    class Status {
    public:
        bool isDwf = false;
        char deviceName[32] = {0};
        char serialNumber[32] = {0};
    } status;

    class RawData {
    public:
        std::vector<double> times, ch1, ch2;
    } rawData;

    class ComplexData {
    public:
        double x, y;
    };
    
    class Buffer {
    public:
        ComplexData ch1, ch2;
    } buffer;
    
    Cfg() {
        rawData.times.resize(SAMPLE_COUNT);
        rawData.ch1.resize(SAMPLE_COUNT);
        rawData.ch2.resize(SAMPLE_COUNT);
        double wdt = 2.0 * PI * excitation.frequency * dt;
        for (int i = 0; i < SAMPLE_COUNT; i++) {
            rawData.times[i] = static_cast<double>(i) * dt;
            rawData.ch1[i] = std::sin(i * wdt);
            rawData.ch2[i] = std::cos(i * wdt);
        }
    }
};