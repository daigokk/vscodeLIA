#pragma once
#include <GLFW/glfw3.h>


class Gui {
public:
    static GLFWwindow* Initialize(const char* title = "");
    
    static void Shutdown(GLFWwindow* window);

    static void BeginFrame(GLFWwindow* window);

    static void EndFrame(GLFWwindow* window);
};
