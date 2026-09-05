#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Gui.h"
#include "Config.h"


void DtWindow(GuiConfig& guiCfg, Config& cfg) {
    if(ImGui::Begin("dt")){
        if (ImPlot::BeginPlot("##dt", ImVec2(-1, -1))) {
            double t_current, t_start;
            int count, idxWrite;
            std::vector<double> times_copy, dts_copy;
            {
                std::lock_guard lock(cfg.ringBuffer.plotMutex);
                const auto& plot = cfg.ringBuffer.plotBuffer;
                t_current = plot.times[plot.idxCurrent];
                t_start = t_current - cfg.ringBuffer.historySec;
                count = plot.nofm < plot.times.size() ? plot.nofm : plot.times.size();
                idxWrite = plot.idxWrite;
                
                times_copy = plot.times;
                dts_copy = plot.dts;
            }

            ImPlotSpec specLine;
            specLine.Offset = idxWrite;
            ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_NoTickLabels);
            ImPlot::SetupAxis(ImAxis_Y1, "dt (s)");
            ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
            auto dt = cfg.ringBuffer.dt * static_cast<double>(cfg.ringBuffer.scopeCfg.nMultiChannel);
            ImPlot::SetupAxisLimits(ImAxis_Y1, dt * 0.9, dt * 1.1, ImGuiCond_Always);
            ImPlot::PlotLine("##dt", times_copy.data(), dts_copy.data(), count, specLine);
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}