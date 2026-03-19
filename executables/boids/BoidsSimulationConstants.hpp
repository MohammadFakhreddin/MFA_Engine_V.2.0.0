#pragma once

struct SimulationConstants
{
    int bEnableSeparationForce{};
    int bEnableAlignmentForce{};
    int bEnableCohesionForce{};
    int bEnableSoftCollisionHandling{};

    int bEnableSoftCollisionForBoundary{};
    int bEnableHardCollisionHandling{};
    int bEnableBoundaryCollisionHandling{};
    float separationRadius{};

    float alignmentRadius{};
    float cohesionRadius{};
    float separationCos{};
    float alignmentCos{};

    float cohesionCos{};
    float separationConstant{};
    float cohesionConstant{};
    float alignmentConstant{};

    int bClampSpeed{};
    float minSpeed{};
    float maxSpeed{};
    int bClampAcc{};

    float minAcc{};
    float maxAcc{};
    float softCollisionOffset{};
    float hardCollisionOffset{};
};
static_assert(sizeof(SimulationConstants) % 16 == 0);