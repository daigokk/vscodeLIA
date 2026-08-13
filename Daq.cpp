#include "Daq.h"

#include <chrono>
#include <cmath>
#include <numbers>
#include <iostream>
#include <thread>

Daq::Daq(Config* cfg) : pCfg_(cfg) {
    initializeDevice();
}

Daq::~Daq() {
    stop();
    closeDevice();
}

void Daq::initializeDevice() {
    try {
        device_data_ = Dwf::Device::open();

        FDwfEnumDeviceName(0, pCfg_->status.deviceName);
        FDwfEnumSN(0, pCfg_->status.serialNumber);
        // Analog input buffer size
        int buffer_size = 0;
        if (FDwfAnalogInBufferSizeInfo(device_data_->handle, 0, &buffer_size) == 0) {
            Dwf::Device::check_error(device_data_);
        }
        printDeviceInfo();
        if(pCfg_->rawData.times.size() > buffer_size){
            std::cout << "Raw buffer size: " << pCfg_->rawData.times.size() << " => " << buffer_size << std::endl;
            pCfg_->RawInit(buffer_size);
        }
        supplies();
        wavegen(pCfg_->excitation.frequency, pCfg_->excitation.amplitude);
        scope.run(device_data_, 1.0 / pCfg_->rawData.dt, static_cast<int>(pCfg_->rawData.times.size()), 0.0, 5.0);
    }
    catch (const Dwf::Error& error) {
        std::cout << "WaveForms error: "
                  << error.instrument << " -> "
                  << error.function << " -> "
                  << error.message << std::endl;
        closeDevice();
    }
}

void Daq::closeDevice() {
    if (device_data_) {
        Dwf::Device::close(device_data_);
        device_data_ = nullptr;
    }
}

void Daq::printDeviceInfo() const {
    std::cout << "WaveForms version: " << device_data_->version << std::endl;
    std::cout << "Device name: " << pCfg_->status.deviceName << std::endl;
    std::cout << "Serial number: " << pCfg_->status.serialNumber << std::endl;
}

void Daq::supplies(const double voltage) {
    /**
    * @brief 電源供給 (V+, V-) を設定する
    * @param voltage 供給電圧 (0~5V, 0.0を指定するとOFFになる)
    */
    bool flag = (voltage != 0.0);
    double abs_voltage = abs(voltage);
    // idxChannel=0: V+ 電圧設定, idxChannel=1: V- 電圧設定
    // idxNode=0: Enable/Disable, idxNode=1: Voltage Level
    for(int idxChannel=0; idxChannel<2; idxChannel++) {
        if (FDwfAnalogIOChannelNodeSet(
            device_data_->handle, idxChannel, 1, 
            (idxChannel % 2==0 ? 1 : -1)*abs_voltage
        ) == 0) {
            Dwf::Device::check_error(device_data_);
        }
        if (FDwfAnalogIOChannelNodeSet(device_data_->handle, idxChannel, 0, flag) == 0) {
            Dwf::Device::check_error(device_data_);
        }   
    }
    // 電源のマスター有効/無効
    if (FDwfAnalogIOEnableSet(device_data_->handle, flag) == 0) {
        Dwf::Device::check_error(device_data_);
    }
}

void Daq::wavegen(const double frequency, const double amplitude, int channel, FUNC function, std::vector<double> data) {
    /**
    * @brief 波形生成器を設定する
    * @param frequency 周波数
    * @param amplitude 振幅
    * @param channel チャンネル (1 or 2)
    * @param function 関数タイプ
    * @param data カスタムデータ
    */
    double offset = 0;
    // enable channel
    channel--;
    if (FDwfAnalogOutNodeEnableSet(device_data_->handle, channel, AnalogOutNodeCarrier, true) == 0) {
        Dwf::Device::check_error(device_data_);
    }
    
    // set function type
    if (FDwfAnalogOutNodeFunctionSet(device_data_->handle, channel, AnalogOutNodeCarrier, function) == 0) {
        Dwf::Device::check_error(device_data_);
    }
    
    // load data if the function type is custom
    if (function == funcCustom) {
        if (FDwfAnalogOutNodeDataSet(device_data_->handle, channel, AnalogOutNodeCarrier, data.data(), data.size()) == 0) {
            Dwf::Device::check_error(device_data_);
        }
    }
    
    // set frequency
    if (FDwfAnalogOutNodeFrequencySet(device_data_->handle, channel, AnalogOutNodeCarrier, frequency) == 0) {
        Dwf::Device::check_error(device_data_);
    }
    
    // set amplitude or DC voltage
    if (FDwfAnalogOutNodeAmplitudeSet(device_data_->handle, channel, AnalogOutNodeCarrier, amplitude) == 0) {
        Dwf::Device::check_error(device_data_);
    }
    
    // set offset
    if (FDwfAnalogOutNodeOffsetSet(device_data_->handle, channel, AnalogOutNodeCarrier, offset) == 0) {
        Dwf::Device::check_error(device_data_);
    }

    // set trigger W1に同期させる。しかしながら、FDwfDeviceTriggerSetの有無によって結果は変わらないようにみえる。
    if (channel != 0) {
        if (FDwfDeviceTriggerSet(device_data_->handle, channel, trigsrcAnalogOut1) == 0) {
            Dwf::Device::check_error(device_data_);
        }
    }
    
    // start
    if (FDwfAnalogOutConfigure(device_data_->handle, -1, true) == 0) {
        Dwf::Device::check_error(device_data_);
    }
}

void Daq::start() {
    thread_ = std::jthread([this](std::stop_token st) {
        if (device_data_) {
            run(st);
        } else {
            runWithoutDaq(st);
        }
    });
}

void Daq::stop() {
    thread_.request_stop();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Daq::run(std::stop_token st) {
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

void Daq::runWithoutDaq(std::stop_token st) {
    pCfg_->status.isRun = true;
    double theta = 0.0;
    const auto loop_period = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(pCfg_->buffer.dt));
    const double angular_step = (100.0 / 180.0) * pCfg_->PI_;
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
