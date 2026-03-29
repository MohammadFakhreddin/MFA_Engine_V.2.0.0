#pragma once

// #include <cstddef>
#include <glm/glm.hpp>

struct FishState
{
    int id{};                       // Fish unique index, starts from 0
    int pad0;
    int pad1;
    int pad2;

    glm::vec3 rbPosition{};
    int pad3;

    glm::vec3 rbVelocity{};
    int pad4;

    glm::vec3 rbForce{};
    int pad5;

    glm::vec3 tPosition{};
    int pad6;

    glm::vec4 tRotation{0.0f, 0.0f, 0.0f, 1.0f};          // Used for animation

    glm::vec3 tScale{1.0f};
    int pad7;

    glm::mat4 tLocalMat4{1.0f};     // The default transform used to adjust the mesh
};
static_assert(sizeof(FishState) % 16 == 0);
static_assert(offsetof(FishState, rbPosition) == 16);
static_assert(offsetof(FishState, rbVelocity) == 32);
static_assert(offsetof(FishState, rbForce) == 48);
static_assert(offsetof(FishState, tPosition) == 64);
static_assert(offsetof(FishState, tRotation) == 80);
static_assert(offsetof(FishState, tScale) == 96);
static_assert(offsetof(FishState, tLocalMat4) == 112);
