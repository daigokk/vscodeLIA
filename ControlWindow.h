#pragma once
#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>
#include "Config.h"
#include "Daq.h"

// DAQの出力する波形(周波数、振幅)を制御する
void ControlWindow(GuiConfig& guiCfg, Config& cfg, Daq& daq) {
    if(!cfg.status.isRun){
        // DAQとの接続が切れたとき
        ImGui::SetNextWindowFocus();
        ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Border, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤
    }
    if(ImGui::Begin("Control")){
        if(!cfg.status.isRun){
            // DAQとの接続が切れたとき
            ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤
            ImGui::Text("Disconnected");
            ImGui::PopStyleColor();
        }
        else{
            ImGui::Text("%s", cfg.status.deviceSerial.c_str());
        }
        ImGui::Separator();
        if(ImGui::BeginTabBar("Wavegen")){
            if(ImGui::BeginTabItem("W1")){
                ImGui::SetNextItemWidth(guiCfg.dpi_scale * 100);
                if(ImGui::SliderFloat("Frequency", &cfg.ringBuffer.sourceChs[0].frequency, 10e3f, 100e3f)) {
                    if(daq.device_data) daq.wavegen(0, cfg.ringBuffer.sourceChs[0].frequency, cfg.ringBuffer.sourceChs[0].amplitude, cfg.ringBuffer.sourceChs[0].phase);
                    cfg.ringBuffer.sourceChs[1].frequency = cfg.ringBuffer.sourceChs[0].frequency;
                    if(daq.device_data) daq.wavegen(1, cfg.ringBuffer.sourceChs[1].frequency, cfg.ringBuffer.sourceChs[1].amplitude, cfg.ringBuffer.sourceChs[1].phase);
                }
                ImGui::SetNextItemWidth(guiCfg.dpi_scale * 100);
                if(ImGui::SliderFloat("Amplitude", &cfg.ringBuffer.sourceChs[0].amplitude, 0.0f, 5.0f)) {    
                    if(daq.device_data) daq.wavegen(0, cfg.ringBuffer.sourceChs[0].frequency, cfg.ringBuffer.sourceChs[0].amplitude, cfg.ringBuffer.sourceChs[0].phase);
                }
                ImGui::BeginDisabled();
                ImGui::SetNextItemWidth(guiCfg.dpi_scale * 100);
                if(ImGui::SliderFloat("Phase", &cfg.ringBuffer.sourceChs[0].phase, -180.0f, 180.0f)) {    
                    if(daq.device_data) daq.wavegen(0, cfg.ringBuffer.sourceChs[0].frequency, cfg.ringBuffer.sourceChs[0].amplitude, cfg.ringBuffer.sourceChs[0].phase);
                }
                ImGui::EndDisabled();
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("W2")){
                ImGui::BeginDisabled();
                ImGui::SetNextItemWidth(guiCfg.dpi_scale * 100);
                if(ImGui::SliderFloat("Frequency", &cfg.ringBuffer.sourceChs[1].frequency, 10e3f, 100e3f)) {
                    if(daq.device_data) daq.wavegen(1, cfg.ringBuffer.sourceChs[1].frequency, cfg.ringBuffer.sourceChs[1].amplitude, cfg.ringBuffer.sourceChs[1].phase);
                }
                ImGui::EndDisabled();
                ImGui::SetNextItemWidth(guiCfg.dpi_scale * 100);
                if(ImGui::SliderFloat("Amplitude", &cfg.ringBuffer.sourceChs[1].amplitude, 0.0f, 5.0f)) {    
                    if(daq.device_data) daq.wavegen(1, cfg.ringBuffer.sourceChs[1].frequency, cfg.ringBuffer.sourceChs[1].amplitude, 0);
                }
                ImGui::SetNextItemWidth(guiCfg.dpi_scale * 100);
                if(ImGui::SliderFloat("Phase", &cfg.ringBuffer.sourceChs[1].phase, -180.0f, 180.0f)) {    
                    if(daq.device_data) daq.wavegen(1, cfg.ringBuffer.sourceChs[1].frequency, cfg.ringBuffer.sourceChs[1].amplitude, cfg.ringBuffer.sourceChs[1].phase);
                }
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("Func")){
                static const char* funcNames[] = { "Sine", "Square", "Triangle" };
                int oldFunc = cfg.ringBuffer.sourceChs[0].func - 1;
                if (ImGui::ListBox("Func", &oldFunc, funcNames, IM_ARRAYSIZE(funcNames), 3)) {
                    cfg.ringBuffer.sourceChs[0].func = oldFunc + 1;
                    //cfg.ringBuffer.sourceChs[1].func = oldFunc + 1;
                    if(daq.device_data) daq.wavegen(0, cfg.ringBuffer.sourceChs[0].frequency, cfg.ringBuffer.sourceChs[0].amplitude, cfg.ringBuffer.sourceChs[0].phase, cfg.ringBuffer.sourceChs[0].func);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::Separator();
        if(ImGui::Button("Auto offset")) {
            cfg.buttonOffsetAutoOnce();
        }
        ImGui::SameLine();
        if(ImGui::Button("Off")) {
            cfg.buttonOffsetOff();
        }
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
        if (ImGui::TreeNode("Phase (Deg.)")) {
            for(int ch = 0; ch < cfg.ringBuffer.offsets.phases_deg.size(); ++ch) {
                ImGui::SetNextItemWidth(guiCfg.dpi_scale * 100);
                if(ImGui::InputFloat(std::format("Ch{}", ch+1).c_str(), &cfg.ringBuffer.offsets.phases_deg[ch], 0.0f, 0.0f, "%.0f")) {
                    
                }
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
    }
    ImGui::End();
    if(!cfg.status.isRun){
        // DAQとの接続が切れたとき
        ImGui::PopStyleColor();
    }
}
