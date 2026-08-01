#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Cfg.h"

void RawWindow(Cfg& cfg) {
    ImGui::Begin("Raw");
    if (ImPlot::BeginPlot("##Raw")) {
        ImPlot::PlotLine("Ch1", cfg.rawData.times.data(), cfg.rawData.ch1.data(), cfg.rawData.times.size());
        ImPlot::PlotLine("Ch2", cfg.rawData.times.data(), cfg.rawData.ch2.data(), cfg.rawData.times.size());
        ImPlot::EndPlot();
    }
    ImGui::End();
}
