#include <format>
#include "Gui.h"
#include "RawWindow.h"
#include "XyWindow.h"
#include "ControlWindow.h"
#include "Cfg.h"
#include "Daq.h"

int main() {
    Cfg cfg;
    Daq daq(&cfg);
    daq.start();
    auto window = Gui::Initialize(
        std::format("codeLIA - {}", cfg.status.serialNumber).c_str()
    );
    
    while (!glfwWindowShouldClose(window)) {
        Gui::BeginFrame(window);

        RawWindow(cfg);
        XyWindow(cfg);
        ControlWindow(cfg, daq);

        Gui::EndFrame(window);
    }

    Gui::Shutdown(window);
    daq.stop();

    return 0;
}
