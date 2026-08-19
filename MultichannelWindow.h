#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Config.h"
#include "Daq.h"

// マルチプレクサ対応
void MultichannelWindow(Config& cfg) {
    if(ImGui::Begin("Multi channel plot")){
        ImPlot::PushColormap(ImPlotColormap_Jet);
        static double scale_limit = 1;
        if (ImPlot::BeginPlot("##Contour Plot")) {
			
            ImPlot::PlotHeatmap(
                "##heatmap",
                cfg.ringBuffer.matrix.data(),
                cfg.ringBuffer.chs.size(),
                cfg.ringBuffer.scheduleTime.size(),
                -scale_limit, scale_limit, nullptr, ImPlotPoint(0, 0),
                ImPlotPoint(cfg.ringBuffer.scheduleTime[cfg.ringBuffer.scheduleTime.size()-1], cfg.ringBuffer.chs.size())
            );
            
            const double t = cfg.ringBuffer.scheduleTime[cfg.ringBuffer.idxCurrent];
            const ImPlotRect limits = ImPlot::GetPlotLimits();
            const double x_line[] = { t, t }, y_line[] = {limits.Y.Min, limits.Y.Max};
            ImPlot::PlotLine("##vline", x_line, y_line, 2);
            ImPlot::EndPlot();
		}
        ImGui::SameLine();
        ImPlot::ColormapScale("Scale", -scale_limit, scale_limit, ImVec2(60, -1), "%g");
        ImPlot::PopColormap();
    }
    ImGui::End();
}


