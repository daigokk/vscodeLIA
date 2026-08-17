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
            for(int i=0; i < cfg.rawData.ch.size(); i++){
                ImPlot::PlotLine(std::format("Ch{}", i+1).c_str(), cfg.rawData.times.data(), cfg.rawData.ch[i].data(), cfg.rawData.times.size());
            }
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}
