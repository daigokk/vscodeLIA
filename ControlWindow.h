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
                if(ImGui::SliderFloat("Frequency", &cfg.ringBuffer.excitation.frequency, 10e3f, 100e3f)) {
                    if(daq.device_data) daq.wavegen(0, cfg.ringBuffer.excitation.frequency, cfg.ringBuffer.excitation.amplitudeCh1, 0);
                }
                if(ImGui::SliderFloat("Amplitude", &cfg.ringBuffer.excitation.amplitudeCh1, 0.1f, 5.0f)) {    
                    if(daq.device_data) daq.wavegen(0, cfg.ringBuffer.excitation.frequency, cfg.ringBuffer.excitation.amplitudeCh1, 0);
                }
                ImGui::EndTabItem();
            }
            if(ImGui::BeginTabItem("W2")){
                ImGui::BeginDisabled();
                if(ImGui::SliderFloat("Frequency", &cfg.ringBuffer.excitation.frequency, 10e3f, 100e3f)) {
                    if(daq.device_data) daq.wavegen(1, cfg.ringBuffer.excitation.frequency, cfg.ringBuffer.excitation.amplitudeCh2, 0);
                }
                ImGui::EndDisabled();
                if(ImGui::SliderFloat("Amplitude", &cfg.ringBuffer.excitation.amplitudeCh2, 0.1f, 5.0f)) {    
                    if(daq.device_data) daq.wavegen(1, cfg.ringBuffer.excitation.frequency, cfg.ringBuffer.excitation.amplitudeCh2, 0);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::Separator();
        if(ImGui::Button("Auto offset")) {
            cfg.ringBuffer.offsets.flag = true;
        }
        ImGui::SameLine();
        if(ImGui::Button("Off")) {
            for(int ch = 0; ch < cfg.ringBuffer.offsets.chs.size(); ++ch){
                cfg.ringBuffer.offsets.chs[ch].real(0);
                cfg.ringBuffer.offsets.chs[ch].imag(0);
            }
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
