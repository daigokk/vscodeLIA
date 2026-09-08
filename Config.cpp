#include "Config.h"
#include <format>
#include <fstream>
#include <iostream>
#include <iomanip> // std::put_time
#include <ctime>   // std::localtime

std::string Config::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;
    if (localtime_s(&local_tm, &now_time) != 0) throw std::runtime_error("localtime_s failed");

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y%m%d%H%M%S");
    return oss.str();
}

bool Config::saveMeasurementResultsToCSV(const std::string& filename) {
    // 測定値をCSVファイルに保存
    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        const char delimiter = ',';
        const auto& plotBuf = ringBuffer.plotBuffer;
        
        // ファイルヘッダー
        outFile << "#t(s)";
        for(int ch = 0; ch < ringBuffer.meaBuffer.chs.size(); ++ch){
            outFile << std::format("{0}ch{1}x{0}ch{1}y", delimiter, ch+1);
        }
        outFile << std::endl;
        // 測定値
        const int size = plotBuf.times.size() < plotBuf.nofm ? plotBuf.times.size() : plotBuf.nofm;
        int startIdx = 0;
        if(plotBuf.nofm > plotBuf.times.size()){
            startIdx = plotBuf.idxWrite;
        }
        for(int i = 0; i < size; ++i){
            int idx = (startIdx + i) % plotBuf.times.size();
            outFile << std::format("{:e}", plotBuf.times[idx]);
            for(int ch = 0; ch < ringBuffer.meaBuffer.chs.size(); ++ch){
                outFile << std::format("{0}{1:e}{0}{2:e}", delimiter, ringBuffer.meaBuffer.chs[ch].xs[idx], ringBuffer.meaBuffer.chs[ch].ys[idx]);
            }
            outFile << std::endl;
        }
        outFile.close();
        return true;
    }
    return false;
}

bool Config::saveSettingsToTxt(const std::string& filename) {
    // 設定値をテキストファイルに保存
    std::ofstream outFile(filename);
    if (outFile.is_open()) {
        outFile << "Raw Rate: " << 1.0 / rawData.rawDt << " Hz" << std::endl;
        outFile << "Number of DAQ Channels: " << ringBuffer.scopeCfg.nDaqChannel << std::endl;
        outFile << "Number of Multiplexer Channels: " << ringBuffer.scopeCfg.nMultiChannel << std::endl;
        outFile << "Ring Buffer dt: " << ringBuffer.dt << " s" << std::endl;
        outFile << "History Duration: " << ringBuffer.historySec << " s" << std::endl;
        outFile << "Wavegen Function: " << ringBuffer.sourceChs[0].func << std::endl;
        outFile << "Wavegen Frequency: " << ringBuffer.sourceChs[0].frequency << " Hz" << std::endl;
        outFile << "Wavegen Amplitude ch1: " << ringBuffer.sourceChs[0].amplitude << " V" << std::endl;
        outFile << "Wavegen Amplitude ch2: " << ringBuffer.sourceChs[1].amplitude << " V" << std::endl;
        outFile << "Wavegen Phase ch2: " << ringBuffer.sourceChs[1].phase << " Deg." << std::endl;
        outFile << "Raw Plot Scale Limits: " << ringBuffer.plotBuffer.rawScaleLimits.Y.Min << ", " << ringBuffer.plotBuffer.rawScaleLimits.Y.Max << std::endl;
        outFile << "Xy Plot Scale Limits X: " << ringBuffer.plotBuffer.xyScaleLimits.X.Min << ", " << ringBuffer.plotBuffer.xyScaleLimits.X.Max << std::endl;
        outFile << "Xy Plot Scale Limits Y: " << ringBuffer.plotBuffer.xyScaleLimits.Y.Min << ", " << ringBuffer.plotBuffer.xyScaleLimits.Y.Max << std::endl;
        outFile << "Multi Plot Scale Limit: " << ringBuffer.plotBuffer.multiScaleLimit << std::endl;
        outFile << "Trigger Level: " << ringBuffer.trigger.level << std::endl;
        outFile.close();
        return true;
    }
    return false;
}

bool Config::loadSettingsFromTxt(const std::string& filename) {
    // 設定値をテキストファイルから読み込み
    std::ifstream inFile(filename);
    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            std::istringstream iss(line);
            std::string key;
            if (std::getline(iss, key, ':')) {
                std::string value1, value2;
                if (std::getline(iss, value1, ',')) {
                    value1.erase(0, value1.find_first_not_of(" \t")); // 前後の空白を削除
                    value1.erase(value1.find_last_not_of(" \t") + 1);
                    if (key == "Raw Rate") {
                        rawData.rawDt = 1.0 / std::stod(value1);
                    } else if (key == "Number of DAQ Channels") {
                        ringBuffer.scopeCfg.nDaqChannel = std::stoi(value1);
                    } else if (key == "Number of Multiplexer Channels") {
                        ringBuffer.scopeCfg.nMultiChannel = std::stoi(value1);
                    } else if (key == "Ring Buffer dt") {
                        ringBuffer.dt = std::stod(value1);
                    } else if (key == "History Duration") {
                        ringBuffer.historySec = std::stod(value1);
                    } else if (key == "Wavegen Function") {
                        ringBuffer.sourceChs[0].func = std::stoi(value1);
                        ringBuffer.sourceChs[1].func = ringBuffer.sourceChs[0].func;
                    } else if (key == "Wavegen Frequency") {
                        ringBuffer.sourceChs[0].frequency = std::stof(value1);
                        ringBuffer.sourceChs[1].frequency = ringBuffer.sourceChs[0].frequency;
                    } else if (key == "Wavegen Amplitude ch1") {
                        ringBuffer.sourceChs[0].amplitude = std::stof(value1);
                    } else if (key == "Wavegen Amplitude ch2") {
                        ringBuffer.sourceChs[1].amplitude = std::stof(value1);
                    } else if (key == "Wavegen Phase ch2") {
                        ringBuffer.sourceChs[1].phase = std::stof(value1);
                    } else if (key == "Raw Plot Scale Limits") {
                        if (std::getline(iss, value2)) {
                            ringBuffer.plotBuffer.rawScaleLimits.Y.Min = std::stof(value1);
                            ringBuffer.plotBuffer.rawScaleLimits.Y.Max = std::stof(value2);
                        }
                    } else if (key == "Xy Plot Scale Limits X") {
                        if (std::getline(iss, value2)) {
                            ringBuffer.plotBuffer.xyScaleLimits.X.Min = std::stof(value1);
                            ringBuffer.plotBuffer.xyScaleLimits.X.Max = std::stof(value2);
                        }
                    } else if (key == "Xy Plot Scale Limits Y") {
                        if (std::getline(iss, value2)) {
                            ringBuffer.plotBuffer.xyScaleLimits.Y.Min = std::stof(value1);
                            ringBuffer.plotBuffer.xyScaleLimits.Y.Max = std::stof(value2);
                        }
                    } else if (key == "Multi Plot Scale Limit") {
                        ringBuffer.plotBuffer.multiScaleLimit = std::stof(value1);
                    } else if (key == "Trigger Level") {
                        ringBuffer.trigger.level = std::stof(value1);
                    }
                }
            }
        }
        inFile.close();
        return true;
    }
    return false;
}

Config::Config() {
    rawData.rawDt = 1.0 / RAW_RATE;
    rawData.times.resize(RAW_SIZE);
    ringBuffer.scopeCfg.nDaqChannel = N_DAQ_CHANNEL;
    ringBuffer.scopeCfg.nMultiChannel = N_MULTIPLEXER_CHANNEL;

    ringBuffer.sourceChs.resize(2);
    ringBuffer.sourceChs[0].frequency = EXCITATION_FREQUENCY;
    ringBuffer.sourceChs[0].amplitude = EXCITATION_AMPLITUDE;
    ringBuffer.sourceChs[0].phase = 0.0;
    ringBuffer.sourceChs[0].func = 1;
    ringBuffer.sourceChs[1].frequency = ringBuffer.sourceChs[0].frequency;
    ringBuffer.sourceChs[1].amplitude = 0.0;
    ringBuffer.sourceChs[1].phase = ringBuffer.sourceChs[0].phase;
    ringBuffer.sourceChs[1].func = ringBuffer.sourceChs[0].func;

    ringBuffer.dt = RINGBUFFER_DT;
    ringBuffer.historySec = HISTORY_SEC;

    ringBuffer.plotBuffer.rawScaleLimits.Y.Min = -1.0;
    ringBuffer.plotBuffer.rawScaleLimits.Y.Max = 1.0;
    ringBuffer.plotBuffer.xyScaleLimits.X.Min = -1.0;
    ringBuffer.plotBuffer.xyScaleLimits.X.Max = 1.0;
    ringBuffer.plotBuffer.xyScaleLimits.Y.Min = -1.0;
    ringBuffer.plotBuffer.xyScaleLimits.Y.Max = 1.0;
    ringBuffer.plotBuffer.multiScaleLimit = 1.0;

    ringBuffer.trigger.level = 0.0;

    loadSettingsFromTxt();
    
    rawData.init(rawData.rawDt, rawData.times.size(), ringBuffer.scopeCfg.nDaqChannel * ringBuffer.scopeCfg.nMultiChannel);
    ringBuffer.initSource(ringBuffer.sourceChs[0].frequency, ringBuffer.sourceChs[0].amplitude, ringBuffer.sourceChs[1].amplitude);
    ringBuffer.init(ringBuffer.dt, ringBuffer.historySec, ringBuffer.scopeCfg.nDaqChannel, ringBuffer.scopeCfg.nMultiChannel);
    fftBuffer.numHarmonics_x.resize(N_HARMONICS);
    fftBuffer.numHarmonics_y.resize(N_HARMONICS);
}

Config::~Config(){
    if(!saveMeasurementResultsToCSV()){
        std::cerr << "Failed to save measurement results to CSV." << std::endl;
    }
    if(!saveSettingsToTxt()){
        std::cerr << "Failed to save settings to text file." << std::endl;
    }
}