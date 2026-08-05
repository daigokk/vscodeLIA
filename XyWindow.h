#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Cfg.h"

void XyWindow(Cfg& cfg) {
    ImGui::Begin("XY");
    if (ImPlot::BeginPlot("##XY")) {
        ImPlot::PlotScatter("PSD", &(cfg.buffer.ch1.xs[0]), &(cfg.buffer.ch1.ys[0]), 1);
        ImPlot::PlotScatter("FFT", cfg.fftBuffer.numHarmonics_x.data(), cfg.fftBuffer.numHarmonics_y.data(), cfg.fftBuffer.numHarmonics_x.size());
        ImPlot::EndPlot();
    }
    ImGui::End();
}
