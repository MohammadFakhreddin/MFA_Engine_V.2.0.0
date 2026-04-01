#pragma once

struct SimulationConstants
{
    int bEnableSeparationForce {};
    float separationRadius {};
    float separationCos {};
    float separationConstant {};
    
    int bEnableAlignmentForce {};
    float alignmentRadius {};
    float alignmentCos {};
    float alignmentConstant {};
    
    int bEnableCohesionForce {};
    float cohesionRadius {};
    float cohesionCos {};
    float cohesionConstant {};
    
    int bEnableSoftCollisionHandling {};
    float softCollisionOffset {};
    int bEnableHardCollisionHandling {};
    float hardCollisionOffset {};
    
    int bClampSpeed {};
    float minSpeed {};
    float maxSpeed {};
    int pad0 {};

    int bClampAcc {};
    float minAcc {};
    float maxAcc {};
    int pad1 {};
};
static_assert(sizeof(SimulationConstants) % 16 == 0, "SimulationConstants must be aligned 16 by 16");