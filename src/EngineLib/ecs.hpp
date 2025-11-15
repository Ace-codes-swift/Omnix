#pragma once

#include <string>

// Component structures for ECS
struct Position {
    float x, y, z;
    Position(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

struct Transform {
    float x, y, z;
    Transform(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

struct Rotation {
    float x, y, z;
    Rotation(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

struct Scale {
    float x, y, z;
    Scale(float x = 1, float y = 1, float z = 1) : x(x), y(y), z(z) {}
};

class ECS {
public:
    void CreateEntity(std::string entityID);
    void DeleteEntity(std::string entityID);
    void AddComponent(std::string entityID, std::string componentName);
    void RemoveComponent(std::string entityID, std::string componentName);
    void GetComponent(std::string entityID, std::string componentName);
    void SetComponent(std::string entityID, std::string componentName, std::string componentValue);
    void GetComponentValue(std::string entityID, std::string componentName);
    void SetComponentValue(std::string entityID, std::string componentName, std::string componentValue);
    void GetEntity(std::string entityID);
    void SetEntity(std::string entityID, std::string entityName);
    void GetEntityValue(std::string entityID, std::string entityName);
    void SetEntityValue(std::string entityID, std::string entityName, std::string entityValue);
};