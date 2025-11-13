#include "Renderer.h"
#include <iostream>

// Cube vertices with positions and colors (positions updated at render-time)
float Renderer::cubeVertices[] = {
    // positions          // colors
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  // front bottom left
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  // front bottom right
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  // front top right
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  // front top left
    
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,  // back bottom left
     0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  // back bottom right
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,  // back top right
    -0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f   // back top left
};

const unsigned int Renderer::cubeIndices[] = {
    // front face
    0, 1, 2,  2, 3, 0,
    // back face
    4, 5, 6,  6, 7, 4,
    // left face
    7, 3, 0,  0, 4, 7,
    // right face
    1, 5, 6,  6, 2, 1,
    // top face
    3, 2, 6,  6, 7, 3,
    // bottom face
    0, 1, 5,  5, 4, 0
};

const char* Renderer::vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vertexColor;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

const char* Renderer::fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

Renderer::Renderer() : VAO(0), VBO(0), EBO(0) {
}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize() {
    // Setup cube geometry
    setupCube();
    
    // Create and compile shaders
    createShaders();
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    return true;
}

void Renderer::setupCube() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void Renderer::createShaders() {
    if (!cubeShader.loadFromSource(vertexShaderSource, fragmentShaderSource)) {
        std::cerr << "Failed to create cube shader!" << std::endl;
    }
}

void Renderer::render(const Camera& camera, float aspectRatio, const UI* ui) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    
    cubeShader.use();
    
    // Update cube vertex positions based on UI size (non-static scaling)
    float sx = 0.5f;
    float sy = 0.5f;
    float sz = 0.5f;
    if (ui != nullptr) {
        sx = ui->getSizeX();
        sy = ui->getSizeY();
        sz = ui->getSizeZ();
    }
    // Update positions (every 6 floats: x,y,z,r,g,b)
    float scaled[] = {
        -sx, -sy, -sz,  1.0f, 0.0f, 0.0f,
         sx, -sy, -sz,  0.0f, 1.0f, 0.0f,
         sx,  sy, -sz,  0.0f, 0.0f, 1.0f,
        -sx,  sy, -sz,  1.0f, 1.0f, 0.0f,
        -sx, -sy,  sz,  1.0f, 0.0f, 1.0f,
         sx, -sy,  sz,  0.0f, 1.0f, 1.0f,
         sx,  sy,  sz,  1.0f, 1.0f, 1.0f,
        -sx,  sy,  sz,  0.5f, 0.5f, 0.5f
    };
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(scaled), scaled);

    // Set transformation matrices
    Mat4 model; // Identity matrix (cube at origin)
    Mat4 view = camera.getViewMatrix();
    Mat4 projection = camera.getProjectionMatrix(aspectRatio);
    
    cubeShader.setMat4("model", model);
    cubeShader.setMat4("view", view);
    cubeShader.setMat4("projection", projection);
    
    // Render cube
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
}
