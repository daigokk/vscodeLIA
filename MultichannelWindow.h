#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Config.h"
#include "Daq.h"

// マルチプレクサ対応
void MultichannelWindow(GuiConfig& guiCfg, Config& cfg, const PlotBufferSnapshot& plot) {
    if(ImGui::Begin("Multi channel plot")){
        ImGui::SetNextItemWidth(guiCfg.dpi_scale * 100);
        bool isMiliV = false;
        if(cfg.ringBuffer.plotBuffer.scaleLimit < 0.1){
            isMiliV = true;
        }
        ImGui::SliderFloat("y (V)", &cfg.ringBuffer.plotBuffer.scaleLimit, 0.01, cfg.rawData.range, "%.2f");
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
        const int count = plot.nofm < plot.times.size() ? plot.nofm : plot.times.size();
        const int idxWrite = plot.idxWrite;
        const int ringSize = (int)plot.times.size();
        const int heatmapRows = (int)plot.ys.size();
        const int heatmapRowsRBF = heatmapRows * cfg.ringBuffer.RBF_K;
        t_current = plot.times[plot.idxCurrent];
        t_start = t_current - cfg.ringBuffer.historySec;
        const auto& times_copy = plot.times;
        const auto& ys_copy = plot.ys;
        const auto& matrix_copy = plot.matrix;
        const auto& matrixRBF_copy = plot.matrixRBF;
        
        if (ImPlot::BeginPlot("##Line Plot", ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight()/3))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time", ImPlotAxisFlags_NoTickLabels);
            if(isMiliV){
                ImPlot::SetupAxis(ImAxis_Y1, "y (mV)");
                ImPlot::SetupAxisFormat(ImAxis_Y1, ImPlotFormatter(Gui::MiliFormatter));
            }
            else{
                ImPlot::SetupAxis(ImAxis_Y1, "y (V)");
            }
            //ImPlot::SetupLegend(ImPlotLocation_East, true);
            ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -cfg.ringBuffer.plotBuffer.scaleLimit, cfg.ringBuffer.plotBuffer.scaleLimit, ImGuiCond_Always);
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
                        -cfg.ringBuffer.plotBuffer.scaleLimit, cfg.ringBuffer.plotBuffer.scaleLimit, nullptr,
                        ImPlotPoint(t_start, heatmapRows), ImPlotPoint(t_current, 0),
                        {ImPlotProp_Offset, idxWrite * heatmapRows,
                         ImPlotProp_Flags, ImPlotHeatmapFlags_ColMajor}
                    );
                    ImPlot::EndPlot();
                }
                ImGui::SameLine();
                if(isMiliV){
                    ImPlot::ColormapScale("y (mV)", -cfg.ringBuffer.plotBuffer.scaleLimit*1e3, cfg.ringBuffer.plotBuffer.scaleLimit*1e3, ImVec2(75, -1), "%g");
                }
                else{
                    ImPlot::ColormapScale("y (V)", -cfg.ringBuffer.plotBuffer.scaleLimit, cfg.ringBuffer.plotBuffer.scaleLimit, ImVec2(75, -1), "%g");
                }
                
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
                        -cfg.ringBuffer.plotBuffer.scaleLimit, cfg.ringBuffer.plotBuffer.scaleLimit, nullptr,
                        ImPlotPoint(t_start, heatmapRows), ImPlotPoint(t_current, 0),
                        {ImPlotProp_Offset, idxWrite * heatmapRowsRBF,
                         ImPlotProp_Flags, ImPlotHeatmapFlags_ColMajor}
                    );
                    ImPlot::EndPlot();
                }
                ImGui::SameLine();
                ImPlot::ColormapScale("y (V)", -cfg.ringBuffer.plotBuffer.scaleLimit, cfg.ringBuffer.plotBuffer.scaleLimit, ImVec2(75, -1), "%g");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImPlot::PopColormap();
    }
    ImGui::End();
}
