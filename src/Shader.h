#pragma once

#include <OpenGL/gl3.h>
#include <string>
#include "Camera.h"

class Shader {
public:
    Shader();
    ~Shader();
    
    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);
    void use();
    
    void setMat4(const std::string& name, const Mat4& matrix);
    void setVec3(const std::string& name, const Vec3& vector);
    void setFloat(const std::string& name, float value);
    void setInt(const std::string& name, int value);
    
    GLuint getProgram() const { return program; }
    
private:
    GLuint program;
    
    GLuint compileShader(const std::string& source, GLenum type);
    bool linkProgram(GLuint vertexShader, GLuint fragmentShader);
    GLint getUniformLocation(const std::string& name);
};
