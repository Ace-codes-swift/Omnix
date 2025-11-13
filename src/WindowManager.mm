#include "WindowManager.h"
#include <iostream>
#include <OpenGL/gl3.h>
#include "Panels.h"

// ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"

// Cocoa headers for native fullscreen
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

@interface PanelsMenuController : NSObject
@end

@implementation PanelsMenuController
- (void)openOmnix:(id)sender { Panels::openOmnix(); }
- (void)openInspector:(id)sender { Panels::openInspector(); }
- (void)openAssets:(id)sender { Panels::openAssets(); }
- (void)openHierarchy:(id)sender { Panels::openHierarchy(); }
- (void)openViewport:(id)sender { Panels::openViewport(); }
- (void)openGameView:(id)sender { Panels::openGameView(); }
@end

static PanelsMenuController* gPanelsController = nil;

// Static member definitions
Camera* WindowManager::cameraPtr = nullptr;
bool WindowManager::firstMouse = true;
float WindowManager::lastX = 0.0f;
float WindowManager::lastY = 0.0f;
bool WindowManager::rightMousePressed = false;
bool WindowManager::middleMousePressed = false;
bool WindowManager::keys[1024] = {false};

WindowManager::WindowManager() : window(nullptr), fullscreen(false), 
    windowedWidth(1024), windowedHeight(768), windowedPosX(100), windowedPosY(100) {
}

WindowManager::~WindowManager() {
    cleanup();
}

bool WindowManager::initialize(int width, int height, const char* title) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Set GLFW hints for OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on macOS
    
    // Create window
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // Store initial windowed size and position
    windowedWidth = width;
    windowedHeight = height;
    glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
    
    // Make context current
    glfwMakeContextCurrent(window);
    
    // Enable vsync
    glfwSwapInterval(1);
    
    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    // Initialize mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
    
    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // Use per-user Application Support path for imgui.ini, seed from bundled default if missing
    if (imguiIniPath.empty()) {
        @autoreleasepool {
            NSArray<NSString*>* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
            NSString* appSupport = (paths.count > 0) ? paths[0] : NSTemporaryDirectory();
            NSString* appDir = [appSupport stringByAppendingPathComponent:@"Omnix"];
            [[NSFileManager defaultManager] createDirectoryAtPath:appDir withIntermediateDirectories:YES attributes:nil error:nil];

            NSString* userIni = [appDir stringByAppendingPathComponent:@"imgui.ini"];
            if (![[NSFileManager defaultManager] fileExistsAtPath:userIni]) {
                NSString* resPath = [[NSBundle mainBundle] resourcePath];
                if (resPath != nil) {
                    NSString* bundledIni = [resPath stringByAppendingPathComponent:@"imgui.ini"];
                    if ([[NSFileManager defaultManager] fileExistsAtPath:bundledIni]) {
                        // Seed user's ini from bundled default
                        [[NSFileManager defaultManager] copyItemAtPath:bundledIni toPath:userIni error:nil];
                    }
                }
            }
            imguiIniPath = [userIni UTF8String];
        }
    }
    io.IniFilename = imguiIniPath.c_str();
    // Load Poppins font from app bundle resources if available
    {
        @autoreleasepool {
            NSString* resPath = [[NSBundle mainBundle] resourcePath];
            if (resPath != nil) {
                NSString* poppath = [resPath stringByAppendingPathComponent:@"Poppins-Regular.ttf"];
                if ([[NSFileManager defaultManager] fileExistsAtPath:poppath]) {
                    ImFontConfig fontCfg;
                    fontCfg.OversampleH = 3;
                    fontCfg.OversampleV = 2;
                    fontCfg.PixelSnapH = true;
                    const float fontSize = 18.0f;
                    ImFont* pop = ImGui::GetIO().Fonts->AddFontFromFileTTF([poppath UTF8String], fontSize, &fontCfg);
                    if (pop) {
                        ImGui::GetIO().FontDefault = pop;
                    }
                }
            }
        }
    }
    // Load any saved layout BEFORE applying our theme, so our theme wins
    if (io.IniFilename && *io.IniFilename) {
        ImGui::LoadIniSettingsFromDisk(io.IniFilename);
    }
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // enable docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // disable multi-viewports for stability
    ImGui::StyleColorsDark();
    // Remove gaps/borders between docked windows
    {
        ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.0f;          // Ensure crisp edges
		style.WindowBorderSize = 0.0f;        // Keep windows borderless
		style.TabBarBorderSize = 0.0f;        // No tab bar bottom border line
		style.DockingSeparatorSize = 0.0f;    // No visible gap between docked nodes
		style.FrameBorderSize = 0.01f;         // Subtle borders on frames/buttons for contrast
		// Center only text inside interactive elements
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.SelectableTextAlign = ImVec2(0.5f, 0.5f);
        

// Near-black navy tones (darker, not fully black)
ImVec4 baseNavy      = ImVec4(0.05f, 0.07f, 0.09f, 1.00f);
ImVec4 hoverNavy     = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
ImVec4 activeNavy    = ImVec4(0.06f, 0.08f, 0.10f, 1.00f);
ImVec4 dimNavy       = ImVec4(0.03f, 0.05f, 0.07f, 1.00f);

// Buttons
style.Colors[ImGuiCol_Button]          = baseNavy;
style.Colors[ImGuiCol_ButtonHovered]   = hoverNavy;
style.Colors[ImGuiCol_ButtonActive]    = activeNavy;

// Tabs
style.Colors[ImGuiCol_Tab]             = activeNavy;  // Inactive tab
style.Colors[ImGuiCol_TabHovered]      = hoverNavy;   // Hovered tab
style.Colors[ImGuiCol_TabActive]       = baseNavy;    // Active tab
style.Colors[ImGuiCol_TabUnfocused]    = dimNavy;     // Unfocused tab
style.Colors[ImGuiCol_TabUnfocusedActive] = activeNavy; // Focused but inactive

// Headers (collapsing headers, selectable, menu items)
style.Colors[ImGuiCol_Header]          = baseNavy;
style.Colors[ImGuiCol_HeaderHovered]   = hoverNavy;
style.Colors[ImGuiCol_HeaderActive]    = activeNavy;

// Frames (input boxes, sliders backgrounds)
style.Colors[ImGuiCol_FrameBg]         = baseNavy;
style.Colors[ImGuiCol_FrameBgHovered]  = hoverNavy;
style.Colors[ImGuiCol_FrameBgActive]   = activeNavy;

// Sliders and grabs
style.Colors[ImGuiCol_SliderGrab]      = hoverNavy;
style.Colors[ImGuiCol_SliderGrabActive]= activeNavy;

// Resize grips
style.Colors[ImGuiCol_ResizeGrip]           = dimNavy;
style.Colors[ImGuiCol_ResizeGripHovered]    = hoverNavy;
style.Colors[ImGuiCol_ResizeGripActive]     = activeNavy;

// Separator hover/active (remove blue tint)
style.Colors[ImGuiCol_SeparatorHovered] = hoverNavy;
style.Colors[ImGuiCol_SeparatorActive]  = activeNavy;

// Title bar active (remove blue)
style.Colors[ImGuiCol_TitleBgActive]    = baseNavy;

// Docking preview highlight (neutral, slightly brighter but non-blue)
style.Colors[ImGuiCol_DockingPreview]   = ImVec4(0.15f, 0.15f, 0.15f, 0.70f);

// Links should not be bright blue in dark UI
#ifdef ImGuiCol_TextLink
style.Colors[ImGuiCol_TextLink]         = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
#endif

// Keep checkmark visible (avoid blue, use light gray for contrast)
style.Colors[ImGuiCol_CheckMark]        = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);

// Selection and nav highlight (avoid blue)
style.Colors[ImGuiCol_TextSelectedBg]   = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
#ifdef ImGuiCol_NavCursor
style.Colors[ImGuiCol_NavCursor]        = hoverNavy;
#else
style.Colors[ImGuiCol_NavHighlight]     = hoverNavy;
#endif

// Border color: match the Viewport L-shape gray (IM_COL32(160,160,160,255))
style.Colors[ImGuiCol_Border]           = ImVec4(0.627f, 0.627f, 0.627f, 1.00f);
style.Colors[ImGuiCol_BorderShadow]     = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

// Make text appear brighter
style.Colors[ImGuiCol_Text]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
style.Colors[ImGuiCol_TextDisabled]     = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
#ifdef ImGuiCol_TextLink
style.Colors[ImGuiCol_TextLink]         = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
#endif
    }
    // If multi-viewports are enabled, flatten window rounding and ensure opaque bg
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Initialize platform/renderer bindings (no auto-install of callbacks)
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    const char* glsl_version = "#version 150"; // macOS GL 3.2+ core profile
    ImGui_ImplOpenGL3_Init(glsl_version);

    // macOS native menu: Panels menu
    if (!gPanelsController) gPanelsController = [PanelsMenuController new];
    NSMenu* mainMenu = [[[NSApplication sharedApplication] mainMenu] retain];
    if (!mainMenu) {
        mainMenu = [[NSMenu alloc] initWithTitle:@""];
        [NSApp setMainMenu:mainMenu];
    }
    NSMenuItem* panelsItem = [[NSMenuItem alloc] initWithTitle:@"Panels" action:nil keyEquivalent:@""];
    NSMenu* panelsMenu = [[NSMenu alloc] initWithTitle:@"Panels"];
    NSMenuItem* omnixItem = [[NSMenuItem alloc] initWithTitle:@"Open Omnix" action:@selector(openOmnix:) keyEquivalent:@""];
    [omnixItem setTarget:gPanelsController];
    [panelsMenu addItem:omnixItem];
    NSMenuItem* cubeItem = [[NSMenuItem alloc] initWithTitle:@"Open Inspector" action:@selector(openInspector:) keyEquivalent:@""];
    [cubeItem setTarget:gPanelsController];
    [panelsMenu addItem:cubeItem];
    NSMenuItem* assetsItem = [[NSMenuItem alloc] initWithTitle:@"Open Assets" action:@selector(openAssets:) keyEquivalent:@""];
    [assetsItem setTarget:gPanelsController];
    [panelsMenu addItem:assetsItem];
    NSMenuItem* hierarchyItem = [[NSMenuItem alloc] initWithTitle:@"Open Hierarchy" action:@selector(openHierarchy:) keyEquivalent:@""];
    [hierarchyItem setTarget:gPanelsController];
    [panelsMenu addItem:hierarchyItem];
    NSMenuItem* viewportItem = [[NSMenuItem alloc] initWithTitle:@"Open Viewport" action:@selector(openViewport:) keyEquivalent:@""];
    [viewportItem setTarget:gPanelsController];
    [panelsMenu addItem:viewportItem];
    NSMenuItem* gameItem = [[NSMenuItem alloc] initWithTitle:@"Open Game View" action:@selector(openGameView:) keyEquivalent:@""];
    [gameItem setTarget:gPanelsController];
    [panelsMenu addItem:gameItem];
    [panelsItem setSubmenu:panelsMenu];
    [[NSApp mainMenu] addItem:panelsItem];

    // Forward character input for text fields
    glfwSetCharCallback(window, charCallback);

    return true;
}

void WindowManager::cleanup() {
    // ImGui shutdown
    if (ImGui::GetCurrentContext() != nullptr) {
        // Persist .ini settings explicitly before shutdown
        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

bool WindowManager::shouldClose() {
    return glfwWindowShouldClose(window);
}

void WindowManager::swapBuffers() {
    glfwSwapBuffers(window);
}

void WindowManager::pollEvents() {
    glfwPollEvents();
    
    // Process continuous input
    static float lastFrame = 0.0f;
    float currentFrame = static_cast<float>(glfwGetTime());
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    processInput(deltaTime);
}

void WindowManager::setFullscreen(bool fs) {
    NSWindow* nswindow = glfwGetCocoaWindow(window);
    bool currentlyFullscreen = ([nswindow styleMask] & NSWindowStyleMaskFullScreen) != 0;
    
    // Only toggle if the desired state is different from current state
    if (fs != currentlyFullscreen) {
        //wait 0.8 seconds
        usleep(800000);
        
        //toggle fullscreen
        [nswindow toggleFullScreen:nil];
    }
    
    fullscreen = fs;
}

bool WindowManager::isFullscreen() const {
    return fullscreen;
}

void WindowManager::getFramebufferSize(int* width, int* height) {
    glfwGetFramebufferSize(window, width, height);
}

float WindowManager::getAspectRatio() {
    int width, height;
    getFramebufferSize(&width, &height);
    return static_cast<float>(width) / static_cast<float>(height);
}

void WindowManager::setInputCallbacks(Camera* camera) {
    cameraPtr = camera;
    
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
}

void WindowManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    WindowManager* wm = static_cast<WindowManager*>(glfwGetWindowUserPointer(window));
    // Forward to ImGui first
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    // Only block app keyboard when typing into a text field; otherwise allow keys to flow
    ImGuiIO& io = ImGui::GetIO();
    bool typing = io.WantTextInput;
    if (typing) return;
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        wm->setFullscreen(!wm->isFullscreen());
    }
    
    // Store key states for continuous input
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            keys[key] = true;
        } else if (action == GLFW_RELEASE) {
            keys[key] = false;
        }
    }
}

void WindowManager::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    // If ImGui wants mouse, do not handle camera
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse && !io.WantTextInput) {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        bool overViewportNoItem = false;
        if (ctx && ctx->HoveredWindow) {
            overViewportNoItem = (strcmp(ctx->HoveredWindow->Name, "Viewport") == 0) && !ImGui::IsAnyItemHovered();
        }
        if (!overViewportNoItem) return;
    }
    // Only process when an active mouse mode is engaged
    if (!rightMousePressed && !middleMousePressed) {
        return;
    }
    // While engaged, re-center the cursor each frame to avoid hitting screen edges
    int ww, wh; glfwGetWindowSize(window, &ww, &wh);
    const double centerX = ww * 0.5;
    const double centerY = wh * 0.5;
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
        return; // consume first event after lock to avoid jump
    }
    
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); // Reversed since y-coordinates go from bottom to top
    
    lastX = static_cast<float>(centerX);
    lastY = static_cast<float>(centerY);
    glfwSetCursorPos(window, centerX, centerY);
    
    if (cameraPtr) {
        // Check if middle mouse button is pressed for camera movement without rotation
        if (middleMousePressed) {
            // Move camera based on mouse movement (fixed inversion)
            float sensitivity = 0.01f;
            Vec3 right = cameraPtr->getRight() * (-xoffset * sensitivity);  // Fixed: re-added negative for correct direction
            Vec3 up = cameraPtr->getUp() * (-yoffset * sensitivity);  // Fixed: added negative for correct direction
            cameraPtr->move(right + up);
        } else if (rightMousePressed) {
            // Mouse look (rotation) - only when right mouse is pressed
            float sensitivity = 0.1f;
            cameraPtr->rotate(xoffset * sensitivity, yoffset * sensitivity);
        }
    }
}

void WindowManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Forward to ImGui first
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse && !io.WantTextInput) {
        bool allowPassThrough = false;
        // If hovering the Viewport window and not hovering any UI item, let the app handle the click
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (ctx != nullptr) {
            ImGuiWindow* hoveredWindow = ctx->HoveredWindow;
            if (hoveredWindow != nullptr) {
                const bool isViewportWindow = (strcmp(hoveredWindow->Name, "Viewport") == 0);
                const bool hoveringAnyItem = ImGui::IsAnyItemHovered();
                if (isViewportWindow && !hoveringAnyItem) {
                    allowPassThrough = true;
                }
            }
        }
        if (!allowPassThrough) {
            return;
        }
    }
    // Determine if we should engage camera input (only when hovering Viewport and not a UI item)
    auto shouldEngageCamera = []() -> bool {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        if (ctx == nullptr) return false;
        ImGuiWindow* hoveredWindow = ctx->HoveredWindow;
        if (hoveredWindow == nullptr) return false;
        const bool isViewportWindow = (strcmp(hoveredWindow->Name, "Viewport") == 0);
        const bool hoveringAnyItem = ImGui::IsAnyItemHovered();
        return isViewportWindow && !hoveringAnyItem;
    };

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            if (shouldEngageCamera()) {
                rightMousePressed = true;
                // Enable raw mouse motion if available for smooth relative input
                if (glfwRawMouseMotionSupported()) {
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                }
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                // Center the cursor to create a stable reference point
                int ww, wh; glfwGetWindowSize(window, &ww, &wh);
                double cx = ww * 0.5, cy = wh * 0.5;
                glfwSetCursorPos(window, cx, cy);
                lastX = static_cast<float>(cx);
                lastY = static_cast<float>(cy);
                firstMouse = false;
            }
        } else if (action == GLFW_RELEASE) {
            if (rightMousePressed) {
                rightMousePressed = false;
                if (!middleMousePressed) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    if (glfwRawMouseMotionSupported()) {
                        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
                    }
                    firstMouse = true;
                }
            }
        }
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            if (shouldEngageCamera()) {
                middleMousePressed = true;
                if (glfwRawMouseMotionSupported()) {
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                }
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                int ww, wh; glfwGetWindowSize(window, &ww, &wh);
                double cx = ww * 0.5, cy = wh * 0.5;
                glfwSetCursorPos(window, cx, cy);
                lastX = static_cast<float>(cx);
                lastY = static_cast<float>(cy);
                firstMouse = false;
            }
        } else if (action == GLFW_RELEASE) {
            if (middleMousePressed) {
                middleMousePressed = false;
                if (!rightMousePressed) {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    if (glfwRawMouseMotionSupported()) {
                        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
                    }
                    firstMouse = true;
                }
            }
        }
    }
}

void WindowManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Forward to ImGui first
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        ImGuiContext* ctx = ImGui::GetCurrentContext();
        bool overViewportNoItem = false;
        if (ctx && ctx->HoveredWindow) {
            overViewportNoItem = (strcmp(ctx->HoveredWindow->Name, "Viewport") == 0) && !ImGui::IsAnyItemHovered();
        }
        if (!overViewportNoItem) return;
    }
    if (cameraPtr) {
        // Use scroll wheel for forward/backward movement like W/S keys
        float velocity = 0.1f; // Reduced sensitivity for smoother movement
        Vec3 movement = cameraPtr->getForward() * (static_cast<float>(yoffset) * velocity);
        cameraPtr->move(movement);
    }
}

void WindowManager::charCallback(GLFWwindow* window, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(window, c);
}

void WindowManager::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void WindowManager::processInput(float deltaTime) {
    // Allow WASD even when ImGui wants keyboard, except when typing text
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;
    if (!cameraPtr) return;
    
    float velocity = 5.0f * deltaTime; // Adjust movement speed
    
    if (keys[GLFW_KEY_W]) {
        cameraPtr->move(cameraPtr->getForward() * velocity);
    }
    if (keys[GLFW_KEY_S]) {
        cameraPtr->move(cameraPtr->getForward() * (-velocity));
    }
    if (keys[GLFW_KEY_A]) {
        cameraPtr->move(cameraPtr->getRight() * (-velocity));
    }
    if (keys[GLFW_KEY_D]) {
        cameraPtr->move(cameraPtr->getRight() * velocity);
    }
    // Removed space/shift up/down movement
}

void WindowManager::beginImGuiFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create a central docking space full-screen
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    // Transparent host so docked windows with NoBackground remain see-through
    ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode; // allow passthrough in central dock, keep docking behavior
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground; // prevent dark tint overlay
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0,0,0,0));
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::PopStyleColor(2);
    // Note: no hardcoded node-id hacks; transparency handled via style/colors and window flags
    // Build macOS menu bar with Panels submenu to toggle panels visibility
    // Removed ImGui top-bar menu; native macOS menu handles panel spawning
    ImGui::End();
}

void WindowManager::renderImGui() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Optional: render platform windows for multi-viewport
    ImGuiIO& io2 = ImGui::GetIO();
    if (io2.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

bool WindowManager::imguiWantsMouse() const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool WindowManager::imguiWantsKeyboard() const {
    // Treat typing state as the blocking condition
    return ImGui::GetIO().WantTextInput;
}
