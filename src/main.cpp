#include <iostream>
#include <OpenGL/gl3.h>
#include "WindowManager.h"
#include "Camera.h"
#include "Renderer.h"
#include "UI.h"

#include "EngineLib/EngineInit.hpp"
#include "EngineLib/Status.hpp"

int main() {
    std::cout << "Starting Omnix..." << std::endl;
    

    bool newProject = true;
    int startEngineMode = 1;
    std::string projectName = "NewProject";
    EngineInit engineInit;
    engineInit.Init(startEngineMode, projectName, newProject);
    
    // Set the project name for the UI file explorer
    UI::setProjectName(projectName);
    
    // Create window manager
    WindowManager windowManager;
    if (!windowManager.initialize(1024, 768, "Omnix")) {
        std::cerr << "Failed to initialize window manager!" << std::endl;
        return -1;
    }
    
    // Create camera
    Camera camera;
    camera.setPosition(Vec3(0.0f, 0.0f, 3.0f));
    
    // Set up input handling
    windowManager.setInputCallbacks(&camera);
    
    // Create renderer
    Renderer renderer;
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return -1;
    }
    
    std::cout << "Omnix initialized successfully!" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  WASD - Move camera" << std::endl;
    std::cout << "  Right Mouse + Move - Look around (camera rotation)" << std::endl;
    std::cout << "  Middle Mouse + Move - Pan camera (no rotation)" << std::endl;
    std::cout << "  Scroll - Move forward/backward" << std::endl;
    std::cout << "  F11 - Toggle fullscreen" << std::endl;
    std::cout << "  Escape - Exit" << std::endl;
    
    // Set initial fullscreen mode
    windowManager.setFullscreen(true);
    
    // Main render loop
    UI ui;
    while (!windowManager.shouldClose()) {
        // Poll events and handle input
        windowManager.pollEvents();
        
        // Start UI frame
        windowManager.beginImGuiFrame();

        // Draw UI
        ui.draw();

        // Get current aspect ratio
        float aspectRatio = windowManager.getAspectRatio();
        
        // Render scene with UI-driven cube scaling
        renderer.render(camera, aspectRatio, &ui);

        // Render UI on top
        windowManager.renderImGui();
        
        // Swap buffers
        windowManager.swapBuffers();
    
    
        std::cout << Status::GetLoadingStatus() << std::endl;
        std::cout << Status::GetRuntimeStatus() << std::endl;
    }
    
    std::cout << "Shutting down Omnix..." << std::endl;
    
    // Cleanup
    renderer.cleanup();
    windowManager.cleanup();
    
    return 0;
}
