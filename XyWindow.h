#pragma once
#include "Gui.h"
#include "Config.h"

#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>

#include <format>

// 位相敏感検波した値を複素平面上に表示する
void XyWindow(GuiConfig& guiCfg, Config& cfg, const PlotBufferSnapshot& plot) {
    ImGui::SetNextWindowPos(ImVec2(guiCfg.dpi_scale*300, guiCfg.dpi_scale*0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(guiCfg.dpi_scale*300, guiCfg.dpi_scale*310), ImGuiCond_FirstUseEver);
    if(ImGui::Begin("XY")){
        if (ImPlot::BeginPlot("##XY", ImVec2(-1, -1), ImPlotFlags_Equal)) {
            ImPlot::SetupAxisLimits(ImAxis_X1, cfg.ringBuffer.plotBuffer.xyScaleLimits.X.Min, cfg.ringBuffer.plotBuffer.xyScaleLimits.X.Max, ImGuiCond_FirstUseEver);
            ImPlot::SetupAxisLimits(ImAxis_Y1, cfg.ringBuffer.plotBuffer.xyScaleLimits.Y.Min, cfg.ringBuffer.plotBuffer.xyScaleLimits.Y.Max, ImGuiCond_FirstUseEver);
            //TODO: ここにラベルを表示するコードを入力
            
            // ここまで
            ImPlotSpec spec;
            const auto& xs_copy = plot.xs;
            const auto& ys_copy = plot.ys;
            const int idxCurrent = plot.idxCurrent;
            const int idxWrite = plot.idxWrite;
            const int ringSize = plot.nofm < plot.times.size() ? plot.nofm : plot.times.size();
            
            spec.Offset = idxWrite;
            for(int ch=0; ch<xs_copy.size(); ch++){
                ImVec4 color = ImPlot::GetColormapColor(ch);
                spec.LineColor = color;
                ImPlot::PlotLine(
                    std::format("##Ch{}", ch+1).c_str(),
                    xs_copy[ch].data(),
                    ys_copy[ch].data(),
                    ringSize,
                    spec
                );
            }
            spec.Offset = 0;
            for(int ch=0; ch<xs_copy.size(); ch++){
                ImVec4 color = ImPlot::GetColormapColor(ch);
                spec.MarkerFillColor = color;
                spec.MarkerLineColor = color;
                ImPlot::PlotScatter(
                    std::format("Ch{}", ch+1).c_str(),
                    &(xs_copy[ch][idxCurrent]),
                    &(ys_copy[ch][idxCurrent]),
                    1,
                    spec
                );
            }
            if(xs_copy.size() == 1) {
                spec.MarkerFillColor = ImVec4(1, 0, 0, 1);
                spec.MarkerLineColor = spec.MarkerFillColor;
                ImPlot::PlotScatter("FFT", cfg.fftBuffer.numHarmonics_x.data(), cfg.fftBuffer.numHarmonics_y.data(), cfg.fftBuffer.numHarmonics_x.size(), spec);
            }
            const ImPlotRect limits = ImPlot::GetPlotLimits();
            cfg.ringBuffer.plotBuffer.xyScaleLimits.X.Max = limits.X.Max;
            cfg.ringBuffer.plotBuffer.xyScaleLimits.X.Min = limits.X.Min;
            cfg.ringBuffer.plotBuffer.xyScaleLimits.Y.Max = limits.Y.Max;
            cfg.ringBuffer.plotBuffer.xyScaleLimits.Y.Min = limits.Y.Min;
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}
