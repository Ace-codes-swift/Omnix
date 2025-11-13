cmake_minimum_required(VERSION 3.20)
project(Omnix)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release configuration
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Static linking for better distribution
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
set(BUILD_SHARED_LIBS OFF)

# Find required packages
find_package(OpenGL REQUIRED)

# Build GLFW statically to include in the app bundle
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Download and build GLFW from source
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# Download Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# Set up source files
set(SOURCES
    src/main.cpp
    src/Camera.cpp
    src/Shader.cpp
    src/Renderer.cpp
    src/Panels.cpp
    src/UI.cpp
    src/WindowManager.mm
)

set(HEADERS
    src/Camera.h
    src/Shader.h
    src/Renderer.h
    src/Panels.h
    src/UI.h
    src/WindowManager.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Build Dear ImGui as a static library (core + GLFW + OpenGL3 backends)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends)
# Silence macOS OpenGL deprecation warnings for ImGui too
target_compile_definitions(imgui PUBLIC GL_SILENCE_DEPRECATION)
# Make sure the OpenGL3 backend uses macOS system OpenGL headers instead of a loader
set_source_files_properties(
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    PROPERTIES COMPILE_FLAGS "-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM -include OpenGL/gl3.h"
)
target_link_libraries(imgui PUBLIC glfw)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    OpenGL::GL
    glfw
    imgui
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework QuartzCore"
)

# Silence OpenGL deprecation warnings on macOS
target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)

# Enable Objective-C++ compilation for .mm files
set_source_files_properties(src/WindowManager.mm PROPERTIES COMPILE_FLAGS "-x objective-c++")

# Set up app bundle
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Omnix"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.omnix"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)

# Create Info.plist for the app bundle
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    @ONLY
)

# Bundle icon: copy resources/Omnix.icns into .app and register with target
set(APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/Omnix.icns)
if (EXISTS ${APP_ICON})
    set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON})
    # Ensure bundle references icon by name without extension via CFBundleIconFile
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_ICON_FILE "Omnix.icns"
    )
endif()

# Ensure a default imgui.ini exists in Resources and is copied into the bundle
# Prefer the checked-in defaults/imgui.ini as the seed
set(IMGUI_DEFAULT_INI ${CMAKE_CURRENT_SOURCE_DIR}/defaults/imgui.ini)
if (EXISTS ${IMGUI_DEFAULT_INI})
    set_source_files_properties(${IMGUI_DEFAULT_INI} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_DEFAULT_INI})
else()
    # Generate a minimal imgui.ini at configure time if not present
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/imgui.ini "[Window][DockSpace]\nPos=0,0\nSize=800,600\nCollapsed=0\n")
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/imgui.ini PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/imgui.ini)
endif()

# Copy Poppins font into app bundle Resources if present
set(POPPINS_FONT ${CMAKE_CURRENT_SOURCE_DIR}/defaults/Poppins-Regular.ttf)
if (EXISTS ${POPPINS_FONT})
    set_source_files_properties(${POPPINS_FONT} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${PROJECT_NAME} PRIVATE ${POPPINS_FONT})
endif()
