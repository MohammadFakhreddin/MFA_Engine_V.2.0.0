#pragma once

#include <glm/glm.hpp>

struct CollisionTriangle
{
    glm::vec3 v0{};
    int placeholder0{};

    glm::vec3 v1{};
    int placeholder1{};

    glm::vec3 v2{};
    int placeholder2{};

    glm::vec3 normal{};
    int placeholder3{};
};
