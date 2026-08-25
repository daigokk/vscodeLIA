#pragma once

#include <DWF/Device.h>
#include <DWF/Scope.h>
#include <DWF/Dio.h>
#include <dwf.h>

#include "Config.h"

// DAQの制御をするクラス
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
    void wavegen(const int channel = 0, const double frequency = 100e3, const double amplitude = 1.0, const double phase = 0.0, FUNC function = Function::sine, std::vector<double> data = std::vector<double>());

    Dwf::Scope scope;
    Dwf::Dio dio;
    Dwf::Device::Data* device_data = nullptr;

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
    
private:
    Config* pCfg_ = nullptr;
    std::jthread thread_;

    void initializeDevice();
    void closeDevice();
    void printDeviceInfo() const;

    void run(std::stop_token st);
    void runWithoutDaq(std::stop_token st);
};
