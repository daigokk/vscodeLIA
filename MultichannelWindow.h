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
        if (ImPlot::BeginPlot("##Line Plot", ImVec2(ImGui::GetWindowWidth()-100, ImGui::GetWindowHeight()/3))) {
            ImPlot::SetupAxis(ImAxis_Y1, "y (V)");
            //ImPlot::SetupLegend(ImPlotLocation_East, true);
            const double t_current = cfg.ringBuffer.scheduleTime[cfg.ringBuffer.scheduleTime.size()-1];
            const double t_start = cfg.ringBuffer.scheduleTime[0];
            ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -scale_limit, scale_limit, ImGuiCond_Always);
            for(int ch = 0; ch < cfg.ringBuffer.chs.size(); ++ch){
                ImPlot::PlotLine(
                    std::format("Ch{}", ch+1).c_str(),
                    cfg.ringBuffer.scheduleTime.data(),
                    cfg.ringBuffer.chs[ch].ys.data(),
                    cfg.ringBuffer.scheduleTime.size() < cfg.ringBuffer.nofm ? cfg.ringBuffer.scheduleTime.size() : cfg.ringBuffer.nofm
                );
            }
            // 現在値を示す縦線
            const double t = cfg.ringBuffer.scheduleTime[cfg.ringBuffer.idxCurrent];
            const double x_line[] = { t, t }, y_line[] = { -scale_limit, scale_limit };
            ImPlot::PlotLine("##Time line", x_line, y_line, 2);

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
                if (ImPlot::BeginPlot("##Contour Plot", ImVec2(ImGui::GetWindowWidth()-100, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
                    ImPlot::SetupAxis(ImAxis_Y1, "Ch");
                    const double t_current = cfg.ringBuffer.scheduleTime[cfg.ringBuffer.scheduleTime.size()-1];
                    const double t_start = cfg.ringBuffer.scheduleTime[0];
                    ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, cfg.ringBuffer.chs.size(), ImGuiCond_Always);
                    ImPlot::PlotHeatmap(
                        "##heatmap",
                        cfg.ringBuffer.matrix.data(),
                        cfg.ringBuffer.chs.size(),
                        cfg.ringBuffer.scheduleTime.size(),
                        -scale_limit, scale_limit, nullptr,
                        ImPlotPoint(0, cfg.ringBuffer.chs.size()),
                        ImPlotPoint(cfg.ringBuffer.scheduleTime[cfg.ringBuffer.scheduleTime.size()-1], 0)
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
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Interpolation")){
                if (ImPlot::BeginPlot("##Interpolation", ImVec2(ImGui::GetWindowWidth()-100, -1))) {
                    ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
                    ImPlot::SetupAxis(ImAxis_Y1, "Ch");
                    const double t_current = cfg.ringBuffer.scheduleTime[cfg.ringBuffer.scheduleTime.size()-1];
                    const double t_start = cfg.ringBuffer.scheduleTime[0];
                    ImPlot::SetupAxisLimits(ImAxis_X1, t_start, t_current, ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, cfg.ringBuffer.chs.size(), ImGuiCond_Always);
                    ImPlot::PlotHeatmap(
                        "##_heatmap",
                        cfg.ringBuffer.matrix2.data(),
                        cfg.ringBuffer.chs.size() * cfg.ringBuffer.RBF_K,
                        cfg.ringBuffer.scheduleTime.size(),
                        -scale_limit, scale_limit, nullptr,
                        ImPlotPoint(0, cfg.ringBuffer.chs.size()),
                        ImPlotPoint(cfg.ringBuffer.scheduleTime[cfg.ringBuffer.scheduleTime.size()-1], 0)
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
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImPlot::PopColormap();
    }
    ImGui::End();
}
