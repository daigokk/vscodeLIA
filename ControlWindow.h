#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Cfg.h"
#include "Daq.h"

void ControlWindow(Cfg& cfg, Daq& daq) {
    ImGui::Begin("Control");
    static float frequency = cfg.excitation.frequency;
    static float amplitude = cfg.excitation.amplitude;
    if(ImGui::SliderFloat("Frequency", &frequency, 1e3f, 100e3f)) {
        cfg.excitation.frequency = frequency;
        daq.wavegen(cfg.excitation.frequency, cfg.excitation.amplitude);
    }
    if(ImGui::SliderFloat("Amplitude", &amplitude, 0.0f, 5.0f)) {
        cfg.excitation.amplitude = amplitude;
        daq.wavegen(cfg.excitation.frequency, cfg.excitation.amplitude);
    }
    if(ImGui::Button("Defaults")) {
        frequency = cfg.excitation.frequency = 100e3f;
        amplitude = cfg.excitation.amplitude = 1.0f;
        daq.wavegen(cfg.excitation.frequency, cfg.excitation.amplitude);
    }
    ImGui::End();
}
