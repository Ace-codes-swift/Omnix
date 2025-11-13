# Omnix

A 3D OpenGL application for macOS that renders a colorful rotating cube with full camera controls.

## Features

- **3D Cube Rendering**: Renders a colorful cube with different colors on each vertex
- **Camera System**: Full 3D camera with view and projection matrices
- **WASD Movement**: Move the camera around the scene
- **Mouse Look**: Hold right mouse button and move to rotate the camera (cursor hidden)
- **Camera Panning**: Hold middle mouse button and move to pan the camera without rotation (cursor hidden)
- **Fullscreen Support**: Native macOS fullscreen mode using Cocoa NSWindow toggleFullScreen
- **Smooth Controls**: Scroll to move forward/backward, full 360° camera rotation

## Controls

| Control | Action |
|---------|--------|
| **W** | Move camera forward |
| **A** | Move camera left |
| **S** | Move camera backward |
| **D** | Move camera right |
| **Right Mouse + Move** | Look around (camera rotation, cursor hidden) |
| **Middle Mouse + Move** | Pan camera (no rotation, cursor hidden) |
| **Scroll Wheel** | Move forward/backward |
| **F11** | Toggle fullscreen |
| **Escape** | Exit application |

## Building

### Prerequisites

- macOS with Xcode Command Line Tools
- CMake 3.20 or higher
- Internet connection (for automatic dependency download during build)

### Dependencies

**No manual installation required!** All dependencies are automatically downloaded and built statically during the build process.

### Build Instructions

1. **Clone and navigate to the project directory**
2. **Run the build script:**
   ```bash
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

## Signing, Notarizing, and Packaging (macOS Distribution)

To distribute on macOS without requiring developer tools on end-user machines, sign, notarize, and staple the DMG:

1. Build the app (`./build.sh`). Ensure `build/Omnix.app` exists.
2. Run the script (replace placeholders):

```bash
./scripts/sign_and_notarize.sh "Developer ID Application: Your Name (TEAMID)" com.example.omnix your-apple-id@example.com 'app-specific-password'
```

Notes:
- Requires a Developer ID Application certificate in your login keychain
- Uses hardened runtime with basic entitlements (see `scripts/entitlements.plist`)
- Uses `notarytool` with Apple ID and an app-specific password
- Produces a notarized, stapled `build/Omnix.dmg` ready for distribution


## Project Structure

```
Omnix/
├── src/
│   ├── main.cpp           # Application entry point
│   ├── Camera.h/cpp       # 3D camera implementation
│   ├── Shader.h/cpp       # OpenGL shader management
│   ├── Renderer.h/cpp     # Cube rendering and OpenGL setup
│   └── WindowManager.h/mm # macOS window and input handling (Obj-C++)
├── CMakeLists.txt         # CMake build configuration
├── Info.plist.in          # macOS app bundle configuration
├── build.sh               # Build script
└── README.md              # This file
```

## Technical Details

- **OpenGL 3.3 Core Profile**: Modern OpenGL with vertex/fragment shaders
- **GLFW**: Cross-platform window and input handling
- **Objective-C++**: For native macOS fullscreen support
- **CMake**: Cross-platform build system
- **macOS App Bundle**: Proper .app package for distribution

## Architecture

- **Camera System**: Implements view and projection matrices with Euler angles
- **Renderer**: Manages VAO/VBO/EBO for cube geometry and shader programs
- **Window Manager**: Handles GLFW window creation, fullscreen, and input callbacks
- **Shader System**: Loads and manages vertex/fragment shaders with uniform handling

The application uses a clean separation of concerns with each component handling its specific responsibilities.
