# Omnix

A macOS-native everything-3D creation studio built with OpenGL, featuring a modular workspace, polyglot scripting hooks, and immersive viewport tooling.

## Overview

Omnix targets every type of 3D experience—games, simulations, digital twins, visualization dashboards, research prototypes, and anything else that needs spatial interaction. The editor centers around a customizable dock space, real-time viewport, and language-agnostic automation pipeline. C# appears today only as a test harness while the runtime evolves to host practically any language through plug-in bridges.

## Features

### Editor Interface
- **Dockable Panels**: Fully customizable workspace with Dear ImGui docking support
- **Multiple View Panels**:
  - **Omnix Panel**: Main control panel with FPS display
  - **Inspector**: Object properties editor with Omnix-style XYZ axis controls
  - **Assets**: Visual script browser with drag-and-drop support
  - **Hierarchy**: Scene object hierarchy (placeholder)
  - **Viewport**: 3D scene view with custom L-shaped ruler guides
  - **Game View**: Runtime experience preview (placeholder)

### Scripting & Automation
- **Language-Agnostic Core**: The runtime exposes neutral data channels so scripts written in any language can drive Omnix. Each connector can surface its own lifecycle (Start/Update/etc.) and map to shared metadata.
- **C# Sample Templates**: A minimal C# template ships today strictly as a pipeline test; it can be replaced with Python, Rust, Lua, Zig, Nim, TypeScript, or any other connector as they come online.
- **Visual Studio Code Integration**: Double-click scripts to open in VS Code (or fall back to the default handler).
- **Drag-and-Drop**: Drag scripts from Assets panel to Inspector
- **Auto-Save Detection**: Monitors script changes and reloads automatically
- **Script Storage**: Scripts saved to `~/Library/Application Support/Omnix/Scripts/`

### 3D Rendering
- **OpenGL 3.3 Core Profile**: Modern graphics pipeline with shaders
- **Interactive Cube**: Colorful test cube with per-vertex coloring
- **Camera System**: Full 3D camera with view and projection matrices
- **Real-time Manipulation**: Adjust cube scale via Inspector XYZ controls

### UI Features
- **Omnix Axis Handles**: Color-coded X (red), Y (green), Z (blue) manipulators with drag-based adjustments
- **Viewport Rulers**: L-shaped guides with mouse-tracking triangular indicators
- **Transparent Panels**: Viewport panel with transparent background
- **Custom Font**: Poppins font for modern UI appearance

## Controls

| Control | Action |
|---------|--------|
| **W/A/S/D** | Move camera around the scene |
| **Right Mouse + Move** | Look around (camera rotation, cursor hidden) |
| **Middle Mouse + Move** | Pan camera (no rotation, cursor hidden) |
| **Scroll Wheel** | Move camera forward/backward |
| **F11** | Toggle fullscreen mode |
| **Escape** | Exit application |
| **Double-click script** | Open in Visual Studio Code |
| **Drag axis labels** | Adjust values by dragging |

## Building

### Prerequisites

- macOS 10.15 (Catalina) or later
- Xcode Command Line Tools
- CMake 3.20 or higher
- Internet connection (for automatic dependency download)

### Dependencies

All dependencies are automatically downloaded and built during compilation:
- **GLFW 3.4**: Window management and input handling
- **Dear ImGui (docking branch)**: Modern immediate-mode GUI
- **EnTT**: Entity-component-system framework (included)

### Build Instructions

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd Omnix
   ```

2. **Run the build script:**
   ```bash
   chmod +x build.sh
   ./build.sh
   ```

3. **Or build manually:**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

### Running

```bash
# From build directory
open Omnix.app

# Or from project root
open build/Omnix.app
```

## Project Structure

```
Omnix/
├── src/                      # Main application source
│   ├── main.cpp             # Entry point and main loop
│   ├── Camera.h/cpp         # 3D camera implementation
│   ├── Shader.h/cpp         # OpenGL shader management
│   ├── Renderer.h/cpp       # 3D rendering and cube setup
│   ├── WindowManager.h/mm   # GLFW window and ImGui integration
│   ├── UI.h/cpp            # Editor UI panels and script management
│   └── Panels.h/cpp        # Panel visibility state management
├── Engine/                  # Core runtime module (future expansion)
│   ├── include/            # Engine headers
│   ├── src/                # Engine implementation
│   └── vendor/             # EnTT entity-component system
├── defaults/               # Default configuration files
│   ├── imgui.ini          # Default ImGui layout
│   └── Poppins-Regular.ttf # UI font
├── resources/              # App resources
│   └── Omnix.icns         # Application icon
├── scripts/                # Build and distribution scripts
│   ├── sign_and_notarize.sh
│   └── package_local.sh
├── CMakeLists.txt         # CMake build configuration
├── Info.plist.in          # macOS app bundle template
└── build.sh               # Automated build script
```

## Technical Architecture

### Core Components

- **Window Manager**: Handles GLFW initialization, ImGui setup, and input callbacks
- **Renderer**: Manages OpenGL state, VAO/VBO/EBO, and shader programs
- **Camera System**: Implements view and projection matrices with Euler angles
- **UI System**: Manages all editor panels and script file operations
- **Shader System**: Loads and manages vertex/fragment shaders with uniform handling

### Design Patterns

- **Immediate Mode GUI**: Using Dear ImGui for responsive, customizable interface
- **Component Architecture**: Modular design with clear separation of concerns
- **Native macOS Integration**: Objective-C++ for platform-specific features

## Distribution

### Code Signing and Notarization

For distribution without requiring developer tools on end-user machines:

```bash
# Set environment variables
export CERT="Developer ID Application: Your Name (TEAMID)"
export BUNDLE_ID="com.yourcompany.omnix"
export APPLE_ID="your-apple-id@example.com"
export APP_PW="app-specific-password"

# Build with signing
./build.sh

# Or sign existing build
./scripts/sign_and_notarize.sh "$CERT" "$BUNDLE_ID" "$APPLE_ID" "$APP_PW"
```

This produces a notarized `build/Omnix.dmg` ready for distribution.

### Requirements
- Developer ID Application certificate in login keychain
- Apple Developer account with notarization capability
- App-specific password for notarization

## Future Development

- Complete Entity Component System integration
- Functional Hierarchy panel for scene management
- Game View with play/pause functionality
- Asset import pipeline
- Project save/load system
- Polyglot scripting connectors (C#, Python, Lua, Rust, Zig, TypeScript, etc.)
- Cross-platform support (Windows/Linux)

## License

[License information to be added]

## Contributing

[Contribution guidelines to be added]

## Acknowledgments

- [Dear ImGui](https://github.com/ocornut/imgui) for the excellent immediate-mode GUI library
- [GLFW](https://www.glfw.org/) for cross-platform window management
- [EnTT](https://github.com/skypjack/entt) for the entity-component system framework