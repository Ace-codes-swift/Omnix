#pragma once

#include <string>


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