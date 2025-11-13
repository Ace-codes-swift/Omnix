#include "Camera.h"
#include <cmath>

// Mat4 implementations
Mat4 Mat4::perspective(float fov, float aspect, float near, float far) {
    Mat4 result;
    float tanHalfFov = tan(fov / 2.0f);
    
    result.m[0] = 1.0f / (aspect * tanHalfFov);
    result.m[5] = 1.0f / tanHalfFov;
    result.m[10] = -(far + near) / (far - near);
    result.m[11] = -1.0f;
    result.m[14] = -(2.0f * far * near) / (far - near);
    result.m[15] = 0.0f;
    
    return result;
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = (center - eye).normalize();
    Vec3 s = f.cross(up).normalize();
    Vec3 u = s.cross(f);
    
    Mat4 result;
    result.m[0] = s.x;
    result.m[4] = s.y;
    result.m[8] = s.z;
    result.m[1] = u.x;
    result.m[5] = u.y;
    result.m[9] = u.z;
    result.m[2] = -f.x;
    result.m[6] = -f.y;
    result.m[10] = -f.z;
    result.m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    result.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    result.m[14] = f.x * eye.x + f.y * eye.y + f.z * eye.z;
    
    return result;
}

Mat4 Mat4::translate(const Vec3& translation) {
    Mat4 result;
    result.m[12] = translation.x;
    result.m[13] = translation.y;
    result.m[14] = translation.z;
    return result;
}

Mat4 Mat4::operator*(const Mat4& other) const {
    Mat4 result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                result.m[i * 4 + j] += m[i * 4 + k] * other.m[k * 4 + j];
            }
        }
    }
    return result;
}

// Camera implementations
Camera::Camera() {
    position = Vec3(0.0f, 0.0f, 3.0f);
    worldUp = Vec3(0.0f, 1.0f, 0.0f);
    yaw = -90.0f;
    pitch = 0.0f;
    fov = 45.0f;
    nearPlane = 0.1f;
    farPlane = 100.0f;
    
    updateVectors();
}

void Camera::setPosition(const Vec3& pos) {
    position = pos;
}

void Camera::setRotation(float y, float p) {
    yaw = y;
    pitch = p;
    
    updateVectors();
}

void Camera::move(const Vec3& direction) {
    position = position + direction;
}

void Camera::rotate(float deltaYaw, float deltaPitch) {
    yaw += deltaYaw;
    pitch += deltaPitch;
    
    updateVectors();
}

Mat4 Camera::getViewMatrix() const {
    return Mat4::lookAt(position, position + front, up);
}

Mat4 Camera::getProjectionMatrix(float aspect) const {
    return Mat4::perspective(fov * M_PI / 180.0f, aspect, nearPlane, farPlane);
}

Vec3 Camera::getForward() const {
    return front;
}

Vec3 Camera::getRight() const {
    return right;
}

Vec3 Camera::getUp() const {
    return up;
}

void Camera::updateVectors() {
    // Calculate the new front vector
    Vec3 newFront;
    newFront.x = cos(yaw * M_PI / 180.0f) * cos(pitch * M_PI / 180.0f);
    newFront.y = sin(pitch * M_PI / 180.0f);
    newFront.z = sin(yaw * M_PI / 180.0f) * cos(pitch * M_PI / 180.0f);
    front = newFront.normalize();
    
    // Calculate right and up vectors
    right = front.cross(worldUp).normalize();
    up = right.cross(front).normalize();
}
