#pragma once

#include <OpenGL/gl3.h>
#include <cmath>

struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }
    
    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }
    
    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
    
    Vec3 normalize() const {
        float length = sqrt(x*x + y*y + z*z);
        if (length > 0.0f) {
            return Vec3(x/length, y/length, z/length);
        }
        return Vec3(0, 0, 0);
    }
    
    Vec3 cross(const Vec3& other) const {
        return Vec3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }
};

struct Mat4 {
    float m[16];
    
    Mat4() {
        // Initialize as identity matrix
        for (int i = 0; i < 16; i++) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    
    static Mat4 perspective(float fov, float aspect, float near, float far);
    static Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
    static Mat4 translate(const Vec3& translation);
    
    Mat4 operator*(const Mat4& other) const;
};

class Camera {
public:
    Camera();
    
    void setPosition(const Vec3& position);
    void setRotation(float yaw, float pitch);
    void move(const Vec3& direction);
    void rotate(float deltaYaw, float deltaPitch);
    
    Mat4 getViewMatrix() const;
    Mat4 getProjectionMatrix(float aspect) const;
    
    Vec3 getForward() const;
    Vec3 getRight() const;
    Vec3 getUp() const;
    
    Vec3 position;
    float yaw;
    float pitch;
    float fov;
    float nearPlane;
    float farPlane;
    
private:
    void updateVectors();
    
    Vec3 front;
    Vec3 right;
    Vec3 up;
    Vec3 worldUp;
};
