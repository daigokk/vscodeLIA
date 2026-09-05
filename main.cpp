#include <format>
#include "Gui.h"
#include "RawWindow.h"
#include "XyWindow.h"
#include "ControlWindow.h"
#include "MultichannelWindow.h"
#include "Dtwindow.h"
#include "Config.h"
#include "Daq.h"
#include "Pipe.h"

bool isPipeMode(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        if (arg == "pipe") {
            return true;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    Config cfg; // 測定に関する設定および測定値を保存するクラス
    Daq daq(&cfg); // DAQの制御をするクラス
    daq.start(); // `daq.stop();`するまでDAQは波形を測定し続ける
    Pipe pipe; // アプリケーション間通信を担当するクラス
    if (isPipeMode(argc, argv)) {
        // パイプモードの場合、コマンドを受信するスレッドを開始する
        pipe.start(&cfg);
    }
    // `Gui`: GLFW、ImGUI、ImPlotの初期設定等を行うクラス
    auto guiCfg = Gui::Initialize(
        std::format("codeLIA - {}", cfg.status.deviceSerial).c_str()
    );
    
    while (!glfwWindowShouldClose(guiCfg.window) && !pipe.endCommandStatus) {
        Gui::BeginFrame(guiCfg.window);
        
        RawWindow(guiCfg, cfg); // DAQが測定した波形を時間軸で表示する
        XyWindow(guiCfg, cfg); // 位相敏感検波した値を複素平面上に表示する
        ControlWindow(guiCfg, cfg, daq); // DAQの出力する波形(周波数、振幅)を制御する
        MultichannelWindow(guiCfg, cfg);
        DtWindow(guiCfg, cfg);
        
        Gui::EndFrame(guiCfg.window);
    }

    pipe.stop();
    daq.stop();
    Gui::Shutdown(guiCfg.window);

    return 0;
}
