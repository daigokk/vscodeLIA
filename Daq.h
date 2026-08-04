#pragma once
#include <WF_SDK/WF_SDK.h>
#include <dwf.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "Cfg.h"
#define PI acos(-1.0)

void psd(Cfg* pCfg){

}


class Daq {
private:
    std::jthread thread_;
    wf::Device::Data *device_data = nullptr;
    Cfg* pCfg;
    void run(std::stop_token st);
    void runWithoutDaq(std::stop_token st);
public:
    Daq(Cfg* cfg) : pCfg(cfg) {
        // do initialization and configuration of the device here
        try {
            // connect to the device
            device_data = wf::device.open();
            FDwfEnumDeviceName(0, pCfg->status.deviceName);
            FDwfEnumSN(0, pCfg->status.serialNumber);
            std::cout << "WaveForms version: " << device_data->version << std::endl;
            std::cout << "Device name: " << pCfg->status.deviceName << std::endl;
            std::cout << "Serial number: " << pCfg->status.serialNumber << std::endl;
            // power supply
            supplies(5);
            // waveform generation
            wavegen(pCfg->excitation.frequency, pCfg->excitation.amplitude);
            // scope setup
            scope(1.0/pCfg->rawData.dt, pCfg->rawData.times.size(), 0, 5);
        }
        catch (wf::Error error) {
            std::cout << "Error: ";
            std::cout << error.instrument << " -> ";
            std::cout << error.function << " -> ";
            std::cout << error.message << std::endl;
            if (device_data) {
                wf::device.close(device_data);
            }
            device_data = nullptr;
        }
    }
    void supplies(const double voltage = 5) {
        wf::Supplies::Data supplies_data;
        supplies_data.master_state = true;
        supplies_data.positive_state = true;
        supplies_data.negative_state = true;
        supplies_data.positive_voltage = voltage;
        supplies_data.negative_voltage = -voltage;
        wf::supplies.switch_(device_data, supplies_data);
    }
    void wavegen(const double frequency = 100e3, const double amplitude = 1) {
        wf::wavegen.generate(device_data, 1, wf::wavegen.function.sine, 0, frequency, amplitude);
    }
    void scope(const double sample_rate = 100e6, const int buffer_size = 10000, const double offset = 0, const double range = 5) {
        wf::scope.open(device_data, sample_rate, buffer_size, offset, range);
        wf::scope.trigger(device_data, true, trigsrcAnalogOut1, 1, 0);
        FDwfAnalogInConfigure(device_data->handle, true, true);
    }
    void start() {
        if(device_data) {
            thread_ = std::jthread([this](std::stop_token st) {
                run(st);
            });
        }
        else{
            thread_ = std::jthread([this](std::stop_token st) {
                runWithoutDaq(st);
            });
        }
    }
    void stop() {
        thread_.request_stop();
        if (thread_.joinable()){
            thread_.join();
        } 
    }
};

void Daq::run(std::stop_token st) {
    // do work until the window requests shutdown
    pCfg->status.isRun = true;
    // ループ開始時刻を基準点として取得
    auto next_time = std::chrono::steady_clock::now();
    for (size_t nloop = 0; !st.stop_requested(); ++nloop) {
        // do something with the device
        STS sts;
        do {
            FDwfAnalogInStatus(device_data->handle, true, &sts);
        } while (sts != stsDone);
        FDwfAnalogInStatusData(device_data->handle, 0, pCfg->rawData.ch1.data(), pCfg->rawData.ch1.size());
        FDwfAnalogInStatusData(device_data->handle, 1, pCfg->rawData.ch2.data(), pCfg->rawData.ch2.size());
        psd(pCfg);

        // 次の予定時刻を計算
        next_time += std::chrono::microseconds((int)(pCfg->buffer.dt * 1e6));
        // 次の予定時刻まで待機
        std::this_thread::sleep_until(next_time);
    }
    wf::device.close(device_data);
    device_data = nullptr;
    pCfg->status.isRun = false;
}

void Daq::runWithoutDaq(std::stop_token st) {
    // Implementation for running without DAQ device
    pCfg->status.isRun = true;
    double theta = 0.0;
    // ループ開始時刻を基準点として取得
    auto next_time = std::chrono::steady_clock::now();
    for (size_t nloop = 0; !st.stop_requested(); ++nloop) {
        theta += (10.0 * pCfg->buffer.dt) / 180.0 * PI; // 1sごとに1度回転するようにthetaを更新
        theta = std::fmod(theta, 2.0 * PI);
        for(int i = 0; i < pCfg->rawData.times.size(); ++i) {
            double t = pCfg->rawData.times[i];
            pCfg->rawData.ch1[i] = std::sin(theta + 2.0 * PI * pCfg->excitation.frequency * t) * pCfg->excitation.amplitude;
        }
        psd(pCfg);

        // 次の予定時刻を計算
        next_time += std::chrono::milliseconds((int)(pCfg->buffer.dt * 1000));
        // 次の予定時刻まで待機
        std::this_thread::sleep_until(next_time);
    }
    pCfg->status.isRun = false;
}