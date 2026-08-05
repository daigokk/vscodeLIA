#pragma once

#include <WF_SDK/WF_SDK.h>
#include <dwf.h>
#include <chrono>
#include <cmath>
#include <numbers>
#include <iostream>
#include <thread>

#include "Cfg.h"
#include <pocketfft_hdronly.h>


inline void fft(Cfg* pCfg) {
    // TODO: ここのフーリエ変換(FFT)のコードを入力
}

inline void psd(Cfg* pCfg) {
    // TODO: ここの位相敏感検波(PSD)のコードを入力
}


class Daq {
public:
    explicit Daq(Cfg* cfg);
    ~Daq();

    Daq(const Daq&) = delete;
    Daq& operator=(const Daq&) = delete;

    void start();
    void stop();

    void supplies(const double voltage = 5.0);
    void wavegen(const double frequency = 100e3, const double amplitude = 1.0, const int channel = 1);
    void scope(const double sample_rate = 100e6, const int buffer_size = 10000, const double offset = 0.0, const double range = 5.0);

private:
    static constexpr double kDefaultVoltage = 5.0;

    Cfg* pCfg_ = nullptr;
    wf::Device::Data* device_data_ = nullptr;
    std::jthread thread_;

    void initializeDevice();
    void closeDevice();
    void printDeviceInfo() const;

    void run(std::stop_token st);
    void runWithoutDaq(std::stop_token st);
};

inline Daq::Daq(Cfg* cfg) : pCfg_(cfg) {
    initializeDevice();
}

inline Daq::~Daq() {
    stop();
    closeDevice();
}

inline void Daq::initializeDevice() {
    try {
        device_data_ = wf::device.open();

        FDwfEnumDeviceName(0, pCfg_->status.deviceName);
        FDwfEnumSN(0, pCfg_->status.serialNumber);

        printDeviceInfo();

        supplies(kDefaultVoltage);
        wavegen(pCfg_->excitation.frequency, pCfg_->excitation.amplitude);
        scope(1.0 / pCfg_->rawData.dt, static_cast<int>(pCfg_->rawData.times.size()), 0.0, 5.0);
    }
    catch (const wf::Error& error) {
        std::cout << "WaveForms error: "
                  << error.instrument << " -> "
                  << error.function << " -> "
                  << error.message << std::endl;

        closeDevice();
    }
}

inline void Daq::closeDevice() {
    if (device_data_) {
        wf::device.close(device_data_);
        device_data_ = nullptr;
    }
}

inline void Daq::printDeviceInfo() const {
    std::cout << "WaveForms version: " << device_data_->version << std::endl;
    std::cout << "Device name: " << pCfg_->status.deviceName << std::endl;
    std::cout << "Serial number: " << pCfg_->status.serialNumber << std::endl;
}

inline void Daq::supplies(const double voltage) {
    wf::Supplies::Data supplies_data;
    supplies_data.master_state = true;
    supplies_data.positive_state = true;
    supplies_data.negative_state = true;
    supplies_data.positive_voltage = voltage;
    supplies_data.negative_voltage = -voltage;

    wf::supplies.switch_(device_data_, supplies_data);
}

inline void Daq::wavegen(const double frequency, const double amplitude, const int channel) {
    wf::wavegen.generate(device_data_, channel, wf::wavegen.function.sine, 0, frequency, amplitude);
}

inline void Daq::scope(const double sample_rate, const int buffer_size, const double offset, const double range) {
    wf::scope.open(device_data_, sample_rate, buffer_size, offset, range);
    wf::scope.trigger(device_data_, true, trigsrcAnalogOut1, 1, 0);
    FDwfAnalogInConfigure(device_data_->handle, true, true);
}

inline void Daq::start() {
    thread_ = std::jthread([this](std::stop_token st) {
        if (device_data_) {
            run(st);
        } else {
            runWithoutDaq(st);
        }
    });
}

inline void Daq::stop() {
    thread_.request_stop();
    if (thread_.joinable()) {
        thread_.join();
    }
}

inline void Daq::run(std::stop_token st) {
    pCfg_->status.isRun = true;
    auto next_time = std::chrono::steady_clock::now();
    const auto loop_period = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(pCfg_->buffer.dt));

    while (!st.stop_requested()) {
        STS sts;
        do {
            FDwfAnalogInStatus(device_data_->handle, true, &sts);
        } while (sts != stsDone);

        FDwfAnalogInStatusData(device_data_->handle, 0, pCfg_->rawData.ch1.data(), pCfg_->rawData.ch1.size());
        FDwfAnalogInStatusData(device_data_->handle, 1, pCfg_->rawData.ch2.data(), pCfg_->rawData.ch2.size());
        psd(pCfg_);

        next_time += loop_period;
        std::this_thread::sleep_until(next_time);
    }

    closeDevice();
    pCfg_->status.isRun = false;
}

inline void Daq::runWithoutDaq(std::stop_token st) {
    pCfg_->status.isRun = true;
    double theta = 0.0;
    const auto loop_period = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(pCfg_->buffer.dt));
    const double angular_step = (10.0 / 180.0) * pCfg_->PI_;
    const auto& times = pCfg_->rawData.times;
    const auto frequency = pCfg_->excitation.frequency;
    const auto amplitude = pCfg_->excitation.amplitude;

    auto next_time = std::chrono::steady_clock::now();

    while (!st.stop_requested()) {
        theta += angular_step * pCfg_->buffer.dt;
        theta = std::fmod(theta, 2.0 * pCfg_->PI_);

        for (size_t i = 0; i < times.size(); ++i) {
            const double wt = 2.0 * pCfg_->PI_ * frequency * times[i];
            pCfg_->rawData.ch1[i] = amplitude * std::sin(wt + theta);
        }

        psd(pCfg_);
        next_time += loop_period;
        std::this_thread::sleep_until(next_time);
    }

    pCfg_->status.isRun = false;
}
