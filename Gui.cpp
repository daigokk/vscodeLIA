#include "Gui.h"
#include <GLFW/glfw3.h>
#include <GL/gl.h>

#include <IMGUI/imgui.h>
#include <IMGUI/imgui_internal.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>
#include <IMGUI/implot.h>

#include <iostream>

static void error_callback(int error, const char* description)
{
    std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void* WindowSettingsHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
    /**
    * @brief ini ファイルからウィンドウの設定を読み込む際に呼び出される関数
    * @param name ini ファイルのエントリ名
    */
    if (strcmp(name, "Data") == 0)
        return (void*)1;
    return NULL;
}

static void WindowSettingsHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
{
    /**
    * @brief ini ファイルからウィンドウの位置とサイズを読み込む
    * @param entry ini ファイルのエントリ
    * @param line ini ファイルの行
    */
    GLFWwindow* window = (GLFWwindow*)ImGui::GetIO().UserData;
    if (!window) return;

    int x, y, w, h;
    if (sscanf(line, "Pos=%d,%d", &x, &y) == 2)
    {
        glfwSetWindowPos(window, x, y);
    }
    else if (sscanf(line, "Size=%d,%d", &w, &h) == 2)
    {
        // 0以下の異常値でウィンドウが消えるのを防止
        if (w > 100 && h > 100) 
            glfwSetWindowSize(window, w, h);
    }
}

static void WindowSettingsHandler_WriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
    /**
    * @brief ini ファイルにウィンドウの位置とサイズを書き出す
    * @param handler ini ファイルのハンドラー
    * @param buf ini ファイルのバッファ
    */
    GLFWwindow* window = (GLFWwindow*)ImGui::GetIO().UserData;
    if (!window) return;

    int x, y, w, h;
    glfwGetWindowPos(window, &x, &y);
    glfwGetWindowSize(window, &w, &h);

    // ini ファイルに書き出すフォーマットを出力
    buf->appendf("[%s][%s]\n", handler->TypeName, "Data");
    buf->appendf("Pos=%d,%d\n", x, y);
    buf->appendf("Size=%d,%d\n", w, h);
    buf->appendf("\n");
}

// ハンドラーの登録関数
void RegisterGLFWWindowSettingsHandler(GLFWwindow* window)
{
    /**
    * @brief GLFWwindow のWindows位置、サイズをimgui.iniファイルから読み込み、保存するハンドラーを ImGui に登録する
    * @param window GLFWwindow のポインタ
    */
    // GLFWの終了処理時にwindowポインタが破棄される回避策として UserData に GLFWwindow のポインタを保持させる
    ImGui::GetIO().UserData = window;

    ImGuiSettingsHandler ini_handler;
    ini_handler.TypeName = "GLFWWindow";
    ini_handler.TypeHash = ImHashStr("GLFWWindow");
    ini_handler.ReadOpenFn = WindowSettingsHandler_ReadOpen;
    ini_handler.ReadLineFn = WindowSettingsHandler_ReadLine;
    ini_handler.WriteAllFn = WindowSettingsHandler_WriteAll;
    
    ImGui::AddSettingsHandler(&ini_handler);
}

GuiConfig Gui::Initialize(const char* title) {
    GuiConfig guiConfg;
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    int xpos, ypos, monitorWidth, monitorHeight;
    glfwGetMonitorWorkarea(monitor, &xpos, &ypos, &monitorWidth, &monitorHeight);

    // モニターのスケールを取得 (GLFW 3.3+)
    guiConfg.monitorScale = ImGui_ImplGlfw_GetContentScaleForMonitor(monitor);

    const char* glsl_version = "#version 130";
    guiConfg.window = glfwCreateWindow(1280, 720, "codeLIA - Dear ImGui", NULL, NULL);
    if (!guiConfg.window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowTitle(guiConfg.window, title);
    glfwSetKeyCallback(guiConfg.window, key_callback);
    glfwMakeContextCurrent(guiConfg.window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(guiConfg.window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    RegisterGLFWWindowSettingsHandler(guiConfg.window);
    return guiConfg;
}

void Gui::Shutdown(GLFWwindow* window){ 
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    if(window){
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

void Gui::BeginFrame(GLFWwindow* window){
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.10f, 0.12f, 0.15f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::EndFrame(GLFWwindow* window){
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    glfwPollEvents();
}
