#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Config.h"
#include "Daq.h"

// DAQの出力する波形(周波数、振幅)を制御する
void ControlWindow(Config& cfg, Daq& daq) {
    if(ImGui::Begin("Control")){
        ImGui::Text("%s", cfg.status.deviceSerial.c_str());
        if(ImGui::SliderFloat("Frequency", &cfg.excitation.frequency, 1e3f, 100e3f)) {
            if(daq.device_data) daq.wavegen(cfg.excitation.frequency, cfg.excitation.amplitude);
        }
        if(ImGui::SliderFloat("Amplitude", &cfg.excitation.amplitude, 0.0f, 5.0f)) {    
            if(daq.device_data) daq.wavegen(cfg.excitation.frequency, cfg.excitation.amplitude);
        }
        if(ImGui::Button("Defaults")) {
            cfg.excitation.frequency = 100e3f;
            cfg.excitation.amplitude = 1.0f;
            if(daq.device_data) daq.wavegen(cfg.excitation.frequency, cfg.excitation.amplitude);
        }
    }
    ImGui::End();
}
