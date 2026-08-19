#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Config.h"

// 位相敏感検波した値を複素平面上に表示する
void XyWindow(Config& cfg) {
    if(ImGui::Begin("XY")){
        if (ImPlot::BeginPlot("##XY")) {
            for(int ch=0; ch<cfg.ringBuffer.chs.size(); ch++){
                ImPlot::PlotScatter(std::format("Ch{}", ch+1).c_str(), &(cfg.ringBuffer.chs[ch].xs[0]), &(cfg.ringBuffer.chs[ch].ys[0]), 1);
            }
            ImPlot::PlotScatter("FFT", cfg.fftBuffer.numHarmonics_x.data(), cfg.fftBuffer.numHarmonics_y.data(), cfg.fftBuffer.numHarmonics_x.size());
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}
