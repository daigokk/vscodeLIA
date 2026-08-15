#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Config.h"

// DAQが測定した波形を時間軸で表示する
void RawWindow(Config& cfg) {
    if(ImGui::Begin("Raw")){
        if (ImPlot::BeginPlot("##Raw")) {
            ImPlot::PlotLine("Ch1", cfg.rawData.times.data(), cfg.rawData.ch1.data(), cfg.rawData.times.size());
            ImPlot::PlotLine("Ch2", cfg.rawData.times.data(), cfg.rawData.ch2.data(), cfg.rawData.times.size());
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}
