#pragma once

#include <glm/glm.hpp>

struct Fish
{
    int id{};                       // Fish unique index, starts from 0

    glm::vec3 rbPosition{};

    glm::vec3 rbVelocity{};
    int placeholder0{};

    glm::vec3 rbForce{};
    int placeholder1{};

    glm::vec3 tPosition{};
    int placeholder2{};

    glm::vec4 tRotation{};          // Used for animation

    glm::vec3 tScale{1.0f};
    int placeholder3{};

    glm::mat4 tLocalMat4{1.0f};     // The default transform used to adjust the mesh

    glm::mat4 tGlobalMat4{1.0f};    // The global transform used for rendering
};
static_assert(sizeof(Fish) % 16 == 0);