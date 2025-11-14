#include "ecs.hpp"
#include "Status.hpp"
#include "entt.hpp"

entt::registry registry;

void ECS::CreateEntity(std::string entityID) {
    
    const auto entity = registry.create();
    registry.emplace<Position>(entity, 0, 0, 0);
    registry.emplace<Transform>(entity, 0, 0, 0);
    registry.emplace<Rotation>(entity, 0, 0, 0);
    registry.emplace<Scale>(entity, 1, 1, 1);
    
    if (registry.valid(entity)) { Status::SetRuntimeStatus("Entity " + entityID + " created");
    
    } else {
        if (!registry.valid(entity)) { Status::SetErrorStatus("Error: Object could not be created");
            return;
        }
    }


    
}
