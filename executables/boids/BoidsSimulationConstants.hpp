#pragma once

// Client-side mirror of the shader structure. Boolean fields stay as bool here for app/UI code.
struct SimulationConstants
{
    bool bEnableSeparationForce{};
    bool bEnableAlignmentForce{};
    bool bEnableCohesionForce{};
    bool bEnableSoftCollisionHandling{};
    bool bEnableSoftCollisionForBoundary{};
    bool bEnableHardCollisionHandling{};
    bool bEnableBoundaryCollisionHandling{};

    float separationRadius{};
    float alignmentRadius{};
    float cohesionRadius{};

    float separationCos{};
    float alignmentCos{};
    float cohesionCos{};

    float separationConstant{};
    float cohesionConstant{};
    float alignmentConstant{};

    bool bClampSpeed{};
    float minSpeed{};
    float maxSpeed{};

    bool bClampAcc{};
    float minAcc{};
    float maxAcc{};

    float softCollisionOffset{};
    float hardCollisionOffset{};
    float strictCollisionOffset{};

    bool bEnableSubMarineSeparationForce{};
    bool bEnableSubMarineAlignmentForce{};
    bool bEnableSubMarineCohesionForce{};

    float subMarineSpeed{};
    float subMarineSeparationRadius{};
    float subMarineAlignmentRadius{};
    float subMarineCohesionRadius{};

    float subMarineSeparationEulerAngle{};
    float subMarineAlignmentEulerAngle{};
    float subMarineCohesionEulerAngle{};

    float subMarineSeparationCos{};
    float subMarineAlignmentCos{};
    float subMarineCohesionCos{};

    float subMarineSeparationConstant{};
    float subMarineAlignmentConstant{};
    float subMarineCohesionConstant{};

    int placeholder0{};
    int placeholder1{};
};
