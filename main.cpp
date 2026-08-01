#include "Gui.h"
#include "RawWindow.h"
#include "XyWindow.h"
#include "Cfg.h"
#include "daq.h"
#include <thread>

int main() {
    Cfg cfg;
    std::jthread daqThread(daq, &cfg);

    auto window = Gui::Initialize();
    
    while (!glfwWindowShouldClose(window)) {
        Gui::BeginFrame(window);

        RawWindow(cfg);
        XyWindow(cfg);

        Gui::EndFrame(window);
    }

    Gui::Shutdown(window);
    daqThread.request_stop();

    return 0;
}
