#pragma once

#include <OpenGL/gl3.h>
#include "Shader.h"
#include "Camera.h"
#include "UI.h"

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool initialize();
    void render(const Camera& camera, float aspectRatio, const UI* ui = nullptr);
    void cleanup();
    
private:
    void setupCube();
    void createShaders();
    
    GLuint VAO, VBO, EBO;
    Shader cubeShader;
    
    static float cubeVertices[];
    static const unsigned int cubeIndices[];
    static const char* vertexShaderSource;
    static const char* fragmentShaderSource;
};
