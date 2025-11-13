#include "Shader.h"
#include <iostream>
#include <vector>

Shader::Shader() : program(0) {
}

Shader::~Shader() {
    if (program != 0) {
        glDeleteProgram(program);
    }
}

bool Shader::loadFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    if (vertexShader == 0) return false;
    
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }
    
    bool success = linkProgram(vertexShader, fragmentShader);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return success;
}

void Shader::use() {
    glUseProgram(program);
}

void Shader::setMat4(const std::string& name, const Mat4& matrix) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix.m);
    }
}

void Shader::setVec3(const std::string& name, const Vec3& vector) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, vector.x, vector.y, vector.z);
    }
}

void Shader::setFloat(const std::string& name, float value) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void Shader::setInt(const std::string& name, int value) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

GLuint Shader::compileShader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* sourceCStr = source.c_str();
    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        
        std::string shaderType = (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        std::cerr << shaderType << " shader compilation failed: " << log.data() << std::endl;
        
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

bool Shader::linkProgram(GLuint vertexShader, GLuint fragmentShader) {
    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(logLength);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        
        std::cerr << "Shader program linking failed: " << log.data() << std::endl;
        
        glDeleteProgram(program);
        program = 0;
        return false;
    }
    
    return true;
}

GLint Shader::getUniformLocation(const std::string& name) {
    return glGetUniformLocation(program, name.c_str());
}
