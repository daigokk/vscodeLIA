#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Gui.h"
#include "Config.h"


void DtWindow(GuiConfig& guiCfg, Config& cfg) {
    ImGui::SetNextWindowPos(ImVec2(guiCfg.dpi_scale*400, guiCfg.dpi_scale*560), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(guiCfg.dpi_scale*200, guiCfg.dpi_scale*190), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("dt")){
        if (ImPlot::BeginPlot("##dt", ImVec2(-1, -1))) {
            const auto& plot = cfg.ringBuffer.plotBuffer;
            const auto count = plot.nofm < plot.times.size() ? plot.nofm : plot.times.size();
            const auto idxWrite = plot.idxWrite;
            const auto t_current = plot.times[plot.idxCurrent];
            const auto t_start = t_current - cfg.ringBuffer.historySec;
            const auto dt = cfg.ringBuffer.dt * static_cast<double>(cfg.ringBuffer.scopeCfg.nMultiChannel);
            
            ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_NoTickLabels);
            ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
            ImPlot::SetupAxis(ImAxis_Y1, "dt (ms)");
            ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(Gui::MiliFormatter));
            ImPlot::SetupAxisLimits(ImAxis_Y1, dt * 0.5, dt * 1.5, ImGuiCond_Always);
            ImPlotSpec spec;
            spec.Offset = plot.idxWrite;
            ImPlot::PlotLine(
                "##dt",
                plot.times.data(),
                plot.dts.data(),
                count, spec);
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}