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
public:
    Daq(Cfg* cfg) : pCfg(cfg) {}
    void supplies(const double voltage = 5) {
        wf::Supplies::Data supplies_data;
        supplies_data.master_state = true;
        supplies_data.positive_state = true;
        supplies_data.negative_state = true;
        supplies_data.positive_voltage = voltage;
        supplies_data.negative_voltage = voltage;
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
        thread_ = std::jthread([this](std::stop_token st) {
            run(st);
        });
    }
    void stop() {
        thread_.request_stop();
        if (thread_.joinable()){
            thread_.join();
        } 
    }
};

void Daq::run(std::stop_token st) {
    try {
        // connect to the device
        device_data = wf::device.open();
        FDwfEnumDeviceName(0, pCfg->status.deviceName);
        FDwfEnumSN(0, pCfg->status.serialNumber);
        std::cout << "WaveForms version: " << device_data->version << std::endl;
        std::cout << "Device name: " << pCfg->status.deviceName << std::endl;
        std::cout << "Serial number: " << pCfg->status.serialNumber << std::endl;

        // do initialization and configuration of the device here
        // power supply
        supplies(5);
        // waveform generation
        wavegen(pCfg->excitation.frequency, pCfg->excitation.amplitude);
        // scope setup
        scope(1.0/pCfg->dt, pCfg->rawData.times.size(), 0, 5);
        // do work until the window requests shutdown
        pCfg->status.isDwf = true;
        for (size_t nloop = 0; !st.stop_requested(); ++nloop) {
            // do something with the device
            STS sts;
            do {
                FDwfAnalogInStatus(device_data->handle, true, &sts);
            } while (sts != stsDone);
            FDwfAnalogInStatusData(device_data->handle, 0, pCfg->rawData.ch1.data(), pCfg->rawData.ch1.size());
            FDwfAnalogInStatusData(device_data->handle, 1, pCfg->rawData.ch2.data(), pCfg->rawData.ch2.size());
            psd(pCfg);
        }
    }
    catch (wf::Error error) {
        std::cout << "Error: ";
        std::cout << error.instrument << " -> ";
        std::cout << error.function << " -> ";
        std::cout << error.message << std::endl;
    }
    if (device_data) {
        wf::device.close(device_data);
    }
    device_data = nullptr;
    pCfg->status.isDwf = false;
}
