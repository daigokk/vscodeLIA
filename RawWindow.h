#pragma once
#include "Gui.h"
#include "Config.h"

#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>

#include <format>

// DAQが測定した波形を時間軸で表示する
void RawWindow(GuiConfig& guiCfg, Config& cfg) {
    ImGui::SetNextWindowPos(ImVec2(guiCfg.dpi_scale*0, guiCfg.dpi_scale*0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(guiCfg.dpi_scale*300, guiCfg.dpi_scale*310), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("Raw")){
        if (ImPlot::BeginPlot("##Raw", ImVec2(-1, -1))) {
            static double y_scale = 1.0;
            //TODO: ここにラベルを表示するコードを入力
            ImPlot::SetupAxis(ImAxis_X1, "time (µs)");
            ImPlot::SetupAxisLimits(ImAxis_X1, cfg.rawData.times.front(), cfg.rawData.times.back(), ImGuiCond_Always);
            ImPlot::SetupAxisFormat(ImAxis_X1, ImPlotFormatter(Gui::MicroFormatter));
            ImPlot::SetupAxis(ImAxis_Y1, y_scale < 0.2 ? "V (mV)" : "V (V)");
            ImPlot::SetupAxisLimits(ImAxis_Y1, cfg.ringBuffer.plotBuffer.rawScaleLimits.Y.Min, cfg.ringBuffer.plotBuffer.rawScaleLimits.Y.Max, ImGuiCond_FirstUseEver);
            if(y_scale < 0.2){
                ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(Gui::MiliFormatter));
            }
            // ここまで
            for(int i=0; i < cfg.rawData.chs.size(); i++){
                ImPlot::PlotLine(std::format("Ch{}", i+1).c_str(), cfg.rawData.times.data(), cfg.rawData.chs[i].data(), cfg.rawData.times.size());
            }
            const ImPlotRect limits = ImPlot::GetPlotLimits();
            cfg.ringBuffer.plotBuffer.rawScaleLimits.Y.Max = limits.Y.Max;
            cfg.ringBuffer.plotBuffer.rawScaleLimits.Y.Min = limits.Y.Min;
            y_scale = limits.Y.Max - limits.Y.Min;
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}
