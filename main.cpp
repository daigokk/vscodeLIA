#include <format>
#include "Gui.h"
#include "RawWindow.h"
#include "XyWindow.h"
#include "ControlWindow.h"
#include "Config.h"
#include "Daq.h"

int main() {
    // 測定に関する設定および測定値を保存するクラス
    Config cfg;
    // DAQの制御をするクラス
    Daq daq(&cfg);
    // `daq.stop();`するまでDAQは波形を測定し続ける
    daq.start();
    // `Gui`: GLFW、ImGUI、ImPlotの初期設定等を行うクラス
    auto window = Gui::Initialize(
        std::format("codeLIA - {}", cfg.status.deviceSerial).c_str()
    );
    
    while (!glfwWindowShouldClose(window)) {
        Gui::BeginFrame(window);

        RawWindow(cfg); // DAQが測定した波形を時間軸で表示する
        XyWindow(cfg); // 位相敏感検波した値を複素平面上に表示する
        ControlWindow(cfg, daq); // DAQの出力する波形(周波数、振幅)を制御する
        
        if(!cfg.status.isRun){
            // DAQとの接続が切れたとき
            ImGui::SetNextWindowFocus();
            if(ImGui::Begin("Error", nullptr, ImGuiWindowFlags_NoSavedSettings)){
                ImGui::Text("Daq got disconnected.");
            }
            ImGui::End();
        }
        Gui::EndFrame(window);
    }

    Gui::Shutdown(window);
    daq.stop();

    return 0;
}
