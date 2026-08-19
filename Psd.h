#pragma once
#include <cstddef>
#include <utility>
#include <vector>
#include <cmath>

class Psd{
private:
    std::vector<double> sin2, cos2;
    std::size_t sampleCount = 0;
    double inverseSampleCount = 0.0;
    const double PI = std::acos(-1.0);
public:
    double frequency = 0.0;
    double dt = 0.0;
    void init(const int N, const double frequency, const double dt);
    std::pair<double, double> calc(const double* inData);
    std::pair<double, double> rotate(const double phase_deg, const double* pInX, const double* pInY);
};

inline void Psd::init(const int N, const double newFrequency, const double newDt){
    if (N <= 0) {
        sin2.clear();
        cos2.clear();
        sampleCount = 0;
        inverseSampleCount = 0.0;
        return;
    }
    frequency = newFrequency;
    dt = newDt;
    sin2.resize(N);
    cos2.resize(N);
    sampleCount = N;

    // 積分範囲`sampleCount`を半周期の整数倍とする
    // const size_t halfPeriodSamples = static_cast<size_t>(0.5 / (frequency * dt));
    // sampleCount = halfPeriodSamples * (N / halfPeriodSamples);
    
    inverseSampleCount = 1.0 / static_cast<double>(sampleCount);

    const double omegaDt = 2.0 * PI * frequency * dt;
    for (int i = 0; i < N; ++i) {
        const double wt = omegaDt * static_cast<double>(i);
        sin2[i] = 2.0 * std::sin(wt);
        cos2[i] = 2.0 * std::cos(wt);
    }
}

inline std::pair<double, double> Psd::calc(const double* inData){
    const size_t N = this->sampleCount;
    const double DT = this->dt;
    const double FREQ = this->frequency;
    double xSum = 0.0;
    double ySum = 0.0;
    // TODO: ここに位相敏感検波のコードを入力

    // ここまで
    return {xSum / N, ySum / N};
}
/*
inline std::pair<double, double> Psd::calc(const double* inData){
    double xSum = 0.0;
    double ySum = 0.0;
    const double* sinValues = sin2.data();
    const double* cosValues = cos2.data();
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const double input = inData[i];
        xSum += input * sinValues[i];
        ySum += input * cosValues[i];
    }
    return {xSum * inverseSampleCount, ySum * inverseSampleCount};
}
*/
inline std::pair<double, double> Psd::rotate(const double phase_deg, const double* pInX, const double* pInY){
    const double theta = phase_deg * (std::acos(-1.0) / 180.0);
    const double sin_t = std::sin(theta);
    const double cos_t = std::cos(theta);
    
    return {*pInY * cos_t - *pInY * sin_t, *pInX * sin_t + *pInY * cos_t};
}
