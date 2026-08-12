#pragma once

#include <WF_SDK/WF_SDK.h>
#include <dwf.h>
#include <chrono>
#include <cmath>
#include <numbers>
#include <iostream>
#include <thread>

#include "Config.h"
#include <pocketfft_hdronly.h>


inline void fft(Config* pCfg) {
    // TODO: ここにフーリエ変換のコードを入力
}

inline void psd(Config* pCfg) {
    // TODO: ここに位相敏感検波のコードを入力
}

class Function {
    /* function names */
    public:
        static const FUNC custom = funcCustom;
        static const FUNC sine = funcSine;
        static const FUNC square = funcSquare;
        static const FUNC triangle = funcTriangle;
        static const FUNC noise = funcNoise;
        static const FUNC dc = funcDC;
        static const FUNC pulse = funcPulse;
        static const FUNC trapezium = funcTrapezium;
        static const FUNC sine_power = funcSinePower;
        static const FUNC ramp_up = funcRampUp;
        static const FUNC ramp_down = funcRampDown;
};

class Daq {
public:
    explicit Daq(Config* cfg);
    ~Daq();
    // コピーを禁止するコード (リソースハンドルを扱うクラスの定石)
    Daq(const Daq&) = delete;
    Daq& operator=(const Daq&) = delete;

    void start();
    void stop();

    void supplies(const double voltage = 5.0);
    void wavegen(const double frequency = 100e3, const double amplitude = 1.0, int channel = 1, FUNC function = Function::sine, std::vector<double> data = std::vector<double>());
    void scope(const double sample_rate = 100e6, const int buffer_size = 10000, const double offset = 0.0, const double range = 5.0);

    class Dio {
    public:
         static void set_mode(wf::Device::Data* device_data, unsigned int fsOutputEnable=0xFFFF) {
            /*
                set a DIO line as input, or as output
                parameters: - device data
                            - True means output, False means input
            */
           if (FDwfDigitalIOOutputEnableSet(device_data->handle, fsOutputEnable) == 0) {
                Daq::check_error(device_data);
            }
        }
        static void set_state(wf::Device::Data* device_data, unsigned int fsOutput) {
            // 設定
            if (FDwfDigitalIOOutputSet(device_data->handle, fsOutput) == 0) {
                Daq::check_error(device_data);
            }
            // 反映
            if (FDwfDigitalIOConfigure(device_data->handle) == 0) {
                Daq::check_error(device_data);
            }
        }
    } dio;

private:
    Config* pCfg_ = nullptr;
    wf::Device::Data* device_data_ = nullptr;
    std::jthread thread_;

    static void check_error(wf::Device::Data *device_data, const char *caller = __builtin_FUNCTION(), const char *file = __FILE__);

    void initializeDevice();
    void closeDevice();
    void printDeviceInfo() const;

    void run(std::stop_token st);
    void runWithoutDaq(std::stop_token st);
};

inline void Daq::check_error(wf::Device::Data *device_data, const char *caller, const char *file) {
    /*
        check for errors
    */
    char err_msg[512];  // variable for the error message
    FDwfGetLastErrorMsg(err_msg);  // get the error message
    device_data->error.message = err_msg;   // cast it to string
    if (device_data->error.message != "") {
        device_data->error.function = caller;
        device_data->error.instrument = file;
        // delete the extension
        size_t index = device_data->error.instrument.find('.');
        if (index != std::string::npos) {
            device_data->error.instrument = device_data->error.instrument.substr(0, index);
        }
        // delete the path
        device_data->error.instrument = std::string(device_data->error.instrument.rbegin(), device_data->error.instrument.rend());
        index = device_data->error.instrument.find('/');
        if (index != std::string::npos) {
            device_data->error.instrument = device_data->error.instrument.substr(0, index);
        }
        index = device_data->error.instrument.find('\\');
        if (index != std::string::npos) {
            device_data->error.instrument = device_data->error.instrument.substr(0, index);
        }
        device_data->error.instrument = std::string(device_data->error.instrument.rbegin(), device_data->error.instrument.rend());
        throw device_data->error;
    }
    return;
}

inline Daq::Daq(Config* cfg) : pCfg_(cfg) {
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
        // Analog input buffer size
        int buffer_size = 0;
        if (FDwfAnalogInBufferSizeInfo(device_data_->handle, 0, &buffer_size) == 0) {
            wf::device.check_error(device_data_);
        }
        printDeviceInfo();
        if(pCfg_->rawData.times.size() > buffer_size){
            std::cout << "Raw buffer size: " << pCfg_->rawData.times.size() << " => " << buffer_size << std::endl;
            pCfg_->RawInit(buffer_size);
        }
        supplies();
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
            check_error(device_data_);
        }
        if (FDwfAnalogIOChannelNodeSet(device_data_->handle, idxChannel, 0, flag) == 0) {
            check_error(device_data_);
        }   
    }
    // 電源のマスター有効/無効
    if (FDwfAnalogIOEnableSet(device_data_->handle, flag) == 0) {
        check_error(device_data_);
    }
}

inline void Daq::wavegen(const double frequency, const double amplitude, int channel, FUNC function, std::vector<double> data) {
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
        check_error(device_data_);
    }
    
    // set function type
    if (FDwfAnalogOutNodeFunctionSet(device_data_->handle, channel, AnalogOutNodeCarrier, function) == 0) {
        check_error(device_data_);
    }
    
    // load data if the function type is custom
    if (function == funcCustom) {
        if (FDwfAnalogOutNodeDataSet(device_data_->handle, channel, AnalogOutNodeCarrier, data.data(), data.size()) == 0) {
            check_error(device_data_);
        }
    }
    
    // set frequency
    if (FDwfAnalogOutNodeFrequencySet(device_data_->handle, channel, AnalogOutNodeCarrier, frequency) == 0) {
        check_error(device_data_);
    }
    
    // set amplitude or DC voltage
    if (FDwfAnalogOutNodeAmplitudeSet(device_data_->handle, channel, AnalogOutNodeCarrier, amplitude) == 0) {
        check_error(device_data_);
    }
    
    // set offset
    if (FDwfAnalogOutNodeOffsetSet(device_data_->handle, channel, AnalogOutNodeCarrier, offset) == 0) {
        check_error(device_data_);
    }
    
    // start
    if (FDwfAnalogOutConfigure(device_data_->handle, -1, true) == 0) {
        check_error(device_data_);
    }
}

inline void Daq::scope(const double sample_rate, const int buffer_size, const double offset, const double range) {
    /**
    * @brief スコープを設定する
    * @param sample_rate サンプルレート
    * @param buffer_size バッファサイズ
    * @param offset オフセット
    * @param range ダイナミックレンジ (5: ±2.5V, 50: ±25V)
    */
    wf::scope.open(device_data_, sample_rate, buffer_size, offset, range);
    wf::scope.trigger(device_data_, true, trigsrcAnalogOut1, 1, 0);
    if (FDwfAnalogInConfigure(device_data_->handle, true, true) == 0) {
        check_error(device_data_);
    }
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
