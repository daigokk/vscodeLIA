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
        static double scale_limit = 1;
        ImPlot::PushColormap(ImPlotColormap_Jet);
        if (ImPlot::BeginPlot("##Contour Plot", ImVec2(ImGui::GetWindowWidth()-100, -1))) {
			ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "Ch");
            // 全チャンネルのy成分をコンター表示
            ImPlot::PlotHeatmap(
                "##heatmap",
                cfg.ringBuffer.matrix.data(),
                cfg.ringBuffer.chs.size(),
                cfg.ringBuffer.scheduleTime.size(),
                -scale_limit, scale_limit, nullptr, ImPlotPoint(0, 0),
                ImPlotPoint(cfg.ringBuffer.scheduleTime[cfg.ringBuffer.scheduleTime.size()-1], cfg.ringBuffer.chs.size())
            );
            // 現在値を示す縦線
            const double t = cfg.ringBuffer.scheduleTime[cfg.ringBuffer.idxCurrent];
            const ImPlotRect limits = ImPlot::GetPlotLimits();
            const double x_line[] = { t, t }, y_line[] = {limits.Y.Min, limits.Y.Max};
            ImPlot::PlotLine("##Time line", x_line, y_line, 2);
            ImPlot::EndPlot();
		}
        ImGui::SameLine();
        ImPlot::ColormapScale("y (V)", -scale_limit, scale_limit, ImVec2(75, -1), "%g");
        ImPlot::PopColormap();
    }
    ImGui::End();
}
