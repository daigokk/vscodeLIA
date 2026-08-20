#include "Daq.h"

#include <chrono>
#include <cmath>
#include <numbers>
#include <iostream>
#include <thread>


Daq::Daq(Config* cfg) : pCfg_(cfg) {
    try {
        device_data = Dwf::Device::open();
        pCfg_->status.deviceSerial = device_data->serial;
        // Analog input buffer size
        int buffer_size = 0;
        if (FDwfAnalogInBufferSizeInfo(device_data->handle, 0, &buffer_size) == 0) {
            Dwf::Device::check_error(device_data);
        }
        printDeviceInfo();
        if(pCfg_->rawData.times.size() > buffer_size){
            std::cout << "Raw buffer size: " << pCfg_->rawData.times.size() << " => " << buffer_size << std::endl;
            pCfg_->rawData.init(pCfg_->rawData.dt, buffer_size, pCfg_->rawData.chs.size());
        }
        dio.set_mode(device_data, 0xffff);
        dio.set_state(device_data, pCfg_->ch_multi);
        supplies();
        wavegen(pCfg_->excitation.frequency, pCfg_->excitation.amplitude);
        scope.run(device_data, 1.0 / pCfg_->rawData.dt, static_cast<int>(pCfg_->rawData.times.size()), 0.0, 5.0);
    }
    catch (const Dwf::Error& error) {
        std::cout << "WaveForms error: "
                  << error.instrument << " -> "
                  << error.function << " -> "
                  << error.message << std::endl;
        closeDevice();
    }
}

Daq::~Daq() {
    stop();
    closeDevice();
}

void Daq::closeDevice() {
    if (device_data) {
        Dwf::Device::close(device_data);
        device_data = nullptr;
    }
}

void Daq::printDeviceInfo() const {
    std::cout << "WaveForms version: " << device_data->version << std::endl;
    std::cout << "Device name: " << device_data->name << std::endl;
    std::cout << "Serial number: " << device_data->serial << std::endl;
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
            device_data->handle, idxChannel, 1, 
            (idxChannel % 2==0 ? 1 : -1)*abs_voltage
        ) == 0) {
            Dwf::Device::check_error(device_data);
        }
        if (FDwfAnalogIOChannelNodeSet(device_data->handle, idxChannel, 0, flag) == 0) {
            Dwf::Device::check_error(device_data);
        }   
    }
    // 電源のマスター有効/無効
    if (FDwfAnalogIOEnableSet(device_data->handle, flag) == 0) {
        Dwf::Device::check_error(device_data);
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
    if (FDwfAnalogOutNodeEnableSet(device_data->handle, channel, AnalogOutNodeCarrier, true) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // set function type
    if (FDwfAnalogOutNodeFunctionSet(device_data->handle, channel, AnalogOutNodeCarrier, function) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // load data if the function type is custom
    if (function == funcCustom) {
        if (FDwfAnalogOutNodeDataSet(device_data->handle, channel, AnalogOutNodeCarrier, data.data(), data.size()) == 0) {
            Dwf::Device::check_error(device_data);
        }
    }
    
    // set frequency
    if (FDwfAnalogOutNodeFrequencySet(device_data->handle, channel, AnalogOutNodeCarrier, frequency) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // set amplitude or DC voltage
    if (FDwfAnalogOutNodeAmplitudeSet(device_data->handle, channel, AnalogOutNodeCarrier, amplitude) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // set offset
    if (FDwfAnalogOutNodeOffsetSet(device_data->handle, channel, AnalogOutNodeCarrier, offset) == 0) {
        Dwf::Device::check_error(device_data);
    }

    // set trigger W1に同期させる。しかしながら、FDwfDeviceTriggerSetの有無によって結果は変わらないようにみえる。
    if (channel != 0) {
        if (FDwfDeviceTriggerSet(device_data->handle, channel, trigsrcAnalogOut1) == 0) {
            Dwf::Device::check_error(device_data);
        }
    }
    
    // start
    if (FDwfAnalogOutConfigure(device_data->handle, -1, true) == 0) {
        Dwf::Device::check_error(device_data);
    }
}

void Daq::start() {
    thread_ = std::jthread([this](std::stop_token st) {
        if (device_data) {
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
    try{
        pCfg_->status.isRun = true;
        const auto loop_period = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double>(pCfg_->ringBuffer.dt));
        
        auto next_time = std::chrono::steady_clock::now();
        while (!st.stop_requested()) {
            STS sts;
            do {
                if (FDwfAnalogInStatus(device_data->handle, true, &sts) == 0) {
                    Dwf::Device::check_error(device_data);
                }
            } while (sts != stsDone);
            
            for(int i=0; i < pCfg_->ringBuffer.chs.size() / N_MULTIPLEXER_CHANNEL; ++i){
                int ch = i + pCfg_->ch_multi * N_DAQ_CHANNEL;
                FDwfAnalogInStatusData(device_data->handle, i, pCfg_->rawData.chs[ch].data(), pCfg_->rawData.chs[ch].size());
            }
            
            pCfg_->update();
            // マルチプレクサのチャンネル更新
            dio.set_state(device_data, pCfg_->ch_multi);
            next_time += loop_period;
            std::this_thread::sleep_until(next_time);
        }
        closeDevice();
        pCfg_->status.isRun = false;
    }
    catch (const Dwf::Error& error) {
        std::cout << "WaveForms error: "
                  << error.instrument << " -> "
                  << error.function << " -> "
                  << error.message << std::endl;
        closeDevice();
        pCfg_->status.isRun = false;
    }
}

void Daq::runWithoutDaq(std::stop_token st) {
    pCfg_->status.isRun = true;
    double theta = 0.0;
    const auto loop_period = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(pCfg_->ringBuffer.dt));
    const double angular_step = (100.0 / 180.0) * pCfg_->PI_;
    const auto& times = pCfg_->rawData.times;
    
    auto next_time = std::chrono::steady_clock::now();
    while (!st.stop_requested()) {
        const auto frequency = pCfg_->excitation.frequency;
        const auto amplitude = pCfg_->excitation.amplitude;
        theta += angular_step * pCfg_->ringBuffer.dt;
        theta = std::fmod(theta, 2.0 * pCfg_->PI_);

        for(int i=0; i < pCfg_->ringBuffer.chs.size() / N_MULTIPLEXER_CHANNEL; ++i){
            int ch = i + pCfg_->ch_multi * N_DAQ_CHANNEL;
            for (size_t j = 0; j < times.size(); ++j) {
                const double wt = 2.0 * pCfg_->PI_ * frequency * times[j] - 360.0/(N_DAQ_CHANNEL*N_MULTIPLEXER_CHANNEL)*ch/180.0*pCfg_->PI_;
                pCfg_->rawData.chs[ch][j] = amplitude * std::sin(wt + theta);
            }
        }

        pCfg_->update();

        next_time += loop_period;
        std::this_thread::sleep_until(next_time);
    }
    pCfg_->status.isRun = false;
}
