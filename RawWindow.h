#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Gui.h"
#include "Config.h"

// DAQが測定した波形を時間軸で表示する
void RawWindow(GuiConfig& guiCfg, Config& cfg) {
    if(ImGui::Begin("Raw")){
        if (ImPlot::BeginPlot("##Raw", ImVec2(-1, -1))) {
            // ここから
            ImPlot::SetupAxis(ImAxis_X1, "time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "V (V)");
            // ここまで
            for(int i=0; i < cfg.rawData.chs.size(); i++){
                ImPlot::PlotLine(std::format("Ch{}", i+1).c_str(), cfg.rawData.times.data(), cfg.rawData.chs[i].data(), cfg.rawData.times.size());
            }
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}
