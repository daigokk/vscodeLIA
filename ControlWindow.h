#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Cfg.h"
#include "Daq.h"

void ControlWindow(Cfg& cfg, Daq& daq) {
    ImGui::Begin("Control");
    ImGui::Text("%s", cfg.status.serialNumber);
    if(ImGui::SliderFloat("Frequency", &cfg.excitation.frequency, 1e3f, 100e3f)) {
        daq.wavegen(cfg.excitation.frequency, cfg.excitation.amplitude);
    }
    if(ImGui::SliderFloat("Amplitude", &cfg.excitation.amplitude, 0.0f, 5.0f)) {    
        daq.wavegen(cfg.excitation.frequency, cfg.excitation.amplitude);
    }
    if(ImGui::Button("Defaults")) {
        cfg.excitation.frequency = 100e3f;
        cfg.excitation.amplitude = 1.0f;
        daq.wavegen(cfg.excitation.frequency, cfg.excitation.amplitude);
    }
    ImGui::End();
}
