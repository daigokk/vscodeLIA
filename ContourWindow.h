#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Config.h"
#include "Daq.h"

// マルチプレクサ対応
void ControlWindow(Config& cfg, Daq& daq) {
    if(ImGui::Begin("Contour Plot")){
        ImPlot::PushColormap(ImPlotColormap_Jet);
        static double scale_limit = 0.01;
        if (ImPlot::BeginPlot("##Contour Plot", ImVec2(640, -1))) {
			/*
            ImPlot::PlotHeatmap(
                "##heatmap",
                config.ringBuffer.data.x.data(),
                config.ringBuffer.pointers.x.size(),
                config.ringBuffer.time.size(),
                -scale_limit, scale_limit, nullptr, ImPlotPoint(0, 0),
                ImPlotPoint(config.ringBuffer.time.size(), config.ringBuffer.pointers.x.size())
            );
            */
            double x_line[] = { 1000, 1000 }, y_line[] = { 0, 8 };
            ImPlot::PlotLine("##vlines", x_line, y_line, 2);
            ImPlot::EndPlot();
		}
        ImGui::SameLine();
        ImPlot::ColormapScale("Scale", -scale_limit, scale_limit, ImVec2(60, -1), "%g");
        ImPlot::PopColormap();
    }
    ImGui::End();
}


