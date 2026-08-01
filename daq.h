#pragma once
#include <WF_SDK/WF_SDK.h>
#include <dwf.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <stop_token>
#include "Cfg.h"
#define PI acos(-1.0)

void psd(Cfg* pCfg){

}

void daq(std::stop_token st, Cfg* pCfg) {
    wf::Device::Data *device_data = nullptr;
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
        wf::Supplies::Data supplies_data;
        supplies_data.master_state = true;
        supplies_data.positive_state = true;
        supplies_data.negative_state = true;
        supplies_data.positive_voltage = 5;
        supplies_data.negative_voltage = 5;
        wf::supplies.switch_(device_data, supplies_data);
        // waveform generation
        wf::wavegen.generate(device_data, 1, wf::wavegen.function.sine, 0, pCfg->excitation.frequency, pCfg->excitation.amplitude, 50, 0, 0, 0);
        // scope setup
        wf::scope.open(device_data, 1.0/pCfg->dt, pCfg->rawData.times.size(), 0, 5);
        wf::scope.trigger(device_data, true, trigsrcAnalogOut1, 1, 0);
        FDwfAnalogInConfigure(device_data->handle, true, true);
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
        wf::scope.close(device_data);
        wf::wavegen.close(device_data);
        wf::supplies.close(device_data);
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
