#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Cfg.h"

void XyWindow(Cfg& cfg) {
    ImGui::Begin("XY");
    if (ImPlot::BeginPlot("##XY")) {
        ImPlot::PlotScatter("##NOW1", &(cfg.buffer.ch1.x), &(cfg.buffer.ch1.y), 1);

        ImPlot::EndPlot();
    }
    ImGui::End();
}
