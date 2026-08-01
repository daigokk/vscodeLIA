#include "Gui.h"
#include "RawWindow.h"
#include "XyWindow.h"
#include "Cfg.h"
#include "Daq.h"

int main() {
    Cfg cfg;
    Daq daq(&cfg);
    daq.start();
    auto window = Gui::Initialize();
    
    while (!glfwWindowShouldClose(window)) {
        Gui::BeginFrame(window);

        RawWindow(cfg);
        XyWindow(cfg);

        Gui::EndFrame(window);
    }

    Gui::Shutdown(window);
    daq.stop();

    return 0;
}
