#ifndef BOIDS_SIMULATION_CONSTANTS
#define BOIDS_SIMULATION_CONSTANTS

struct SimulationConstants
{
    int bEnableSeparationForce;
    float separationRadius;
    float separationCos;
    float separationConstant;
    
    int bEnableAlignmentForce;
    float alignmentRadius;
    float alignmentCos;
    float alignmentConstant;
    
    int bEnableCohesionForce;
    float cohesionRadius;
    float cohesionCos;
    float cohesionConstant;
    
    int bEnableSoftCollisionHandling;
    float softCollisionOffset;
    int bEnableHardCollisionHandling;
    float hardCollisionOffset;
    
    int bClampSpeed;
    float minSpeed;
    float maxSpeed;
    int pad0;

    int bClampAcc;
    float minAcc;
    float maxAcc;
    int pad1;

    // int bEnableSubMarineSeparationForce;
    // int bEnableSubMarineAlignmentForce;
    // int bEnableSubMarineCohesionForce;

    // float subMarineSpeed;
    // float subMarineSeparationRadius;
    // float subMarineAlignmentRadius;
    // float subMarineCohesionRadius;

    // float subMarineSeparationEulerAngle;
    // float subMarineAlignmentEulerAngle;
    // float subMarineCohesionEulerAngle;

    // float subMarineSeparationCos;
    // float subMarineAlignmentCos;
    // float subMarineCohesionCos;

    // float subMarineSeparationConstant;
    // float subMarineAlignmentConstant;
    // float subMarineCohesionConstant;
};

#endif
