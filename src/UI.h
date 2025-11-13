#pragma once

// Lightweight UI facade for Dear ImGui widgets
// Window/context setup and per-frame begin/render are owned by WindowManager.

class UI {
public:
    UI() = default;
    ~UI() = default;

    // Build all ImGui widgets for the current frame
    void draw();
    float getSizeX() const { return scale[0]; }
    float getSizeY() const { return scale[1]; }
    float getSizeZ() const { return scale[2]; }

private:
   
    float scale[3] = {0.5f, 0.5f, 0.5f};
};


