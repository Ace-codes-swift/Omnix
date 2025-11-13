#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include "Camera.h"

class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    bool initialize(int width, int height, const char* title);
    void cleanup();
    
    bool shouldClose();
    void swapBuffers();
    void pollEvents();
    
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const;
    
    void getFramebufferSize(int* width, int* height);
    float getAspectRatio();
    
    // Input handling
    void setInputCallbacks(Camera* camera);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void charCallback(GLFWwindow* window, unsigned int c);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    
    GLFWwindow* getWindow() { return window; }

    // ImGui helpers
    void beginImGuiFrame();
    void renderImGui();
    bool imguiWantsMouse() const;
    bool imguiWantsKeyboard() const;
    
private:
    GLFWwindow* window;
    bool fullscreen;
    int windowedWidth, windowedHeight;
    int windowedPosX, windowedPosY;
    
    // Input state
    static Camera* cameraPtr;
    static bool firstMouse;
    static float lastX, lastY;
    static bool rightMousePressed;
    static bool middleMousePressed;
    static bool keys[1024];
    std::string imguiIniPath;
    
    void processInput(float deltaTime);
};
