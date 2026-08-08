#pragma once
#include <GLFW/glfw3.h>


class Gui {
public:
    // GLFWwindow の初期化と ImGui のセットアップを行う関数。ループ開始前に呼び出す必要がある
    static GLFWwindow* Initialize(const char* title = "");
    // GLFWwindow の終了処理と ImGui のクリーンアップを行う関数。ループ終了後に呼び出す必要がある
    static void Shutdown(GLFWwindow* window);
    // ImGui のフレーム開始処理を行う関数。ループの最初に呼び出す必要がある
    static void BeginFrame(GLFWwindow* window);
    // ImGui のフレーム終了処理を行う関数。ループの最後に呼び出す必要がある
    static void EndFrame(GLFWwindow* window);
};
