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
        // バッファデータをローカル変数にコピー
        double t_current, t_start;
        int count, idxWrite, ringSize, heatmapRows, heatmapRowsRBF;
        std::vector<double> times_copy;
        std::vector<std::vector<double>> ys_copy;
        std::vector<double> matrix_copy, matrixRBF_copy;
        
        {
            std::lock_guard lock(cfg.ringBuffer.plotMutex);
            const auto& plot = cfg.ringBuffer.plotBuffer;
            t_current = plot.times[plot.idxCurrent];
            t_start = t_current - cfg.ringBuffer.historySec;
            count = plot.nofm < plot.times.size() ? plot.nofm : plot.times.size();
            idxWrite = plot.idxWrite;
            ringSize = (int)plot.times.size();
            heatmapRows = (int)plot.ys.size();
            heatmapRowsRBF = heatmapRows * cfg.ringBuffer.RBF_K;
            
            times_copy = plot.times;
            ys_copy = plot.ys;
            matrix_copy = plot.matrix;
            matrixRBF_copy = plot.matrixRBF;
        }
        
        if (ImPlot::BeginPlot("##Line Plot", ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight()/3))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_NoTickLabels);
            ImPlot::SetupAxis(ImAxis_Y1, "y (V)");
            //ImPlot::SetupLegend(ImPlotLocation_East, true);
            ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -scale_limit, scale_limit, ImGuiCond_Always);
            ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
            ImPlotSpec specLine;
            specLine.Offset = idxWrite;
            for(int ch = 0; ch < ys_copy.size(); ++ch){
                ImPlot::PlotLine(
                    std::format("Ch{}", ch+1).c_str(),
                    times_copy.data(),
                    ys_copy[ch].data(),
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
            if(ImGui::BeginTabItem("Original")){
                if (ImPlot::BeginPlot("##Contour Plot", ImVec2(ImGui::GetWindowWidth() - guiCfg.dpi_scale * 100, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_NoTickLabels);
                    ImPlot::SetupAxis(ImAxis_Y1, "Ch");
                    ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, heatmapRows, ImGuiCond_Always);
                    ImPlot::PlotHeatmap(
                        "##heatmap", matrix_copy.data(), heatmapRows, ringSize,
                        -scale_limit, scale_limit, nullptr,
                        ImPlotPoint(t_start, heatmapRows), ImPlotPoint(t_current, 0),
                        {ImPlotProp_Offset, idxWrite * heatmapRows,
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
                        "##_heatmap", matrixRBF_copy.data(), heatmapRowsRBF, ringSize,
                        -scale_limit, scale_limit, nullptr,
                        ImPlotPoint(t_start, heatmapRows), ImPlotPoint(t_current, 0),
                        {ImPlotProp_Offset, idxWrite * heatmapRowsRBF,
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
