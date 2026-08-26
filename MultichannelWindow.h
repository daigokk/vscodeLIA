#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Config.h"
#include "Daq.h"

// マルチプレクサ対応
void MultichannelWindow(GuiConfig& guiCfg, Config& cfg) {
    if(ImGui::Begin("Multi channel plot")){
        static float scale_limit = 1;
        ImGui::SetNextItemWidth(guiCfg.dpi_scale * 100);
        ImGui::SliderFloat("y (V)", &scale_limit, 0.01, cfg.rawData.range, "%.2f");
        ImGui::SameLine();
        ImGui::Checkbox(
            cfg.ringBuffer.trigger.flag ?
                (cfg.ringBuffer.trigger.readyFlag ?
                    (cfg.ringBuffer.trigger.countFlag ?
                        "On" : "Ready")
                    : (cfg.ringBuffer.pauseFlag ?
                        "Trigger" : "Wait"))
                : "Trigger",
            &cfg.ringBuffer.trigger.flag
        );
        ImGui::SameLine();
        if(ImGui::Button(cfg.ringBuffer.pauseFlag ? "Run" : "Pause")) {
            if(cfg.ringBuffer.pauseFlag){
                // Runボタンが押されたとき
                cfg.buttonRun();
            }
            else{
                // Pauseボタンが押されたとき
                cfg.buttonPause();
            }
        }
        std::lock_guard lock(cfg.ringBuffer.plotMutex);
        const auto& plot = cfg.ringBuffer.plotBuffers[cfg.ringBuffer.plotActive.load(std::memory_order_acquire)];
        const double t_current = plot.times[plot.idxCurrent];
        const double t_start = t_current - cfg.ringBuffer.historySec;
        const int count = plot.nofm < plot.times.size() ? plot.nofm : plot.times.size();
        if (ImPlot::BeginPlot("##Line Plot", ImVec2(ImGui::GetWindowWidth() - guiCfg.dpi_scale * 100, ImGui::GetWindowHeight()/3))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_NoTickLabels);
            ImPlot::SetupAxis(ImAxis_Y1, "y (V)");
            //ImPlot::SetupLegend(ImPlotLocation_East, true);
            ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -scale_limit, scale_limit, ImGuiCond_Always);
            ImPlotSpec specLine;
            specLine.Offset = plot.idxWrite;
            for(int ch = 0; ch < plot.ys.size(); ++ch){
                ImPlot::PlotLine(
                    std::format("Ch{}", ch+1).c_str(),
                    plot.times.data(),
                    plot.ys[ch].data(),
                    count,
                    specLine
                );
            }

            // Trigger level
            if(cfg.ringBuffer.trigger.flag){
                const ImPlotRect limits = ImPlot::GetPlotLimits();
                const double x_line[] = { limits.X.Min, limits.X.Max }, y_line[] = {cfg.ringBuffer.trigger.level, cfg.ringBuffer.trigger.level};
                ImPlotSpec specLine;
                specLine.LineColor = ImVec4(1, 0, 0, 1); // Red
                ImPlot::PlotLine("Trigger", x_line, y_line, 2, specLine);
            }
            
            // Events
            if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // plot内をクリックしたとき
                ImPlotPoint mousePos = ImPlot::GetPlotMousePos();
                if (cfg.ringBuffer.trigger.flag) {
                    cfg.ringBuffer.trigger.level = mousePos.y;
                    cfg.ringBuffer.trigger.readyFlag = false;
                    cfg.ringBuffer.trigger.countFlag = false;
                    cfg.ringBuffer.trigger.nofm = 0;
                }
            }
            ImPlot::EndPlot();
        }
        // 全チャンネルのy成分をコンター表示
        ImPlot::PushColormap(ImPlotColormap_Jet);
        if(ImGui::BeginTabBar("Contour")){
            const int ringSize = (int)plot.times.size();
            const int heatmapRows = (int)plot.ys.size();
            const int heatmapRows2 = heatmapRows * cfg.ringBuffer.RBF_K;
            if(ImGui::BeginTabItem("Original")){
                if (ImPlot::BeginPlot("##Contour Plot", ImVec2(ImGui::GetWindowWidth() - guiCfg.dpi_scale * 100, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_NoTickLabels);
                    ImPlot::SetupAxis(ImAxis_Y1, "Ch");
                    ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, heatmapRows, ImGuiCond_Always);
                    ImPlot::PlotHeatmap(
                        "##heatmap", plot.matrix.data(), heatmapRows, ringSize,
                        -scale_limit, scale_limit, nullptr,
                        ImPlotPoint(t_start, heatmapRows), ImPlotPoint(t_current, 0),
                        {ImPlotProp_Offset, plot.idxWrite * heatmapRows,
                         ImPlotProp_Flags, ImPlotHeatmapFlags_ColMajor}
                    );
                    ImPlot::EndPlot();
                }
                ImGui::SameLine();
                ImPlot::ColormapScale("y (V)", -scale_limit, scale_limit, ImVec2(75, -1), "%g");
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Interpolation")){
                if (ImPlot::BeginPlot("##Interpolation", ImVec2(ImGui::GetWindowWidth()-100, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_NoTickLabels);
                    ImPlot::SetupAxis(ImAxis_Y1, "Ch");
                    ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, heatmapRows, ImGuiCond_Always);
                    ImPlot::PlotHeatmap(
                        "##_heatmap", plot.matrix2.data(), heatmapRows2, ringSize,
                        -scale_limit, scale_limit, nullptr,
                        ImPlotPoint(t_start, heatmapRows), ImPlotPoint(t_current, 0),
                        {ImPlotProp_Offset, plot.idxWrite * heatmapRows2,
                         ImPlotProp_Flags, ImPlotHeatmapFlags_ColMajor}
                    );
                    ImPlot::EndPlot();
                }
                ImGui::SameLine();
                ImPlot::ColormapScale("y (V)", -scale_limit, scale_limit, ImVec2(75, -1), "%g");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImPlot::PopColormap();
    }
    ImGui::End();
}
