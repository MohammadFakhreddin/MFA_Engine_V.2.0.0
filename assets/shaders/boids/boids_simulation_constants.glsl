#ifndef BOIDS_SIMULATION_CONSTANTS
#define BOIDS_SIMULATION_CONSTANTS

struct SimulationConstants
{
    int bEnableSeparationForce;
    int bEnableAlignmentForce;
    int bEnableCohesionForce;
    int bEnableSoftCollisionHandling;
    int bEnableSoftCollisionForBoundary;
    int bEnableHardCollisionHandling;
    int bEnableBoundaryCollisionHandling;

    float separationRadius;
    float alignmentRadius;
    float cohesionRadius;

    float separationCos;
    float alignmentCos;
    float cohesionCos;

    float separationConstant;
    float cohesionConstant;
    float alignmentConstant;

    int bClampSpeed;
    float minSpeed;
    float maxSpeed;

    int bClampAcc;
    float minAcc;
    float maxAcc;

    float softCollisionOffset;
    float hardCollisionOffset;
    float strictCollisionOffset;

    int bEnableSubMarineSeparationForce;
    int bEnableSubMarineAlignmentForce;
    int bEnableSubMarineCohesionForce;

    float subMarineSpeed;
    float subMarineSeparationRadius;
    float subMarineAlignmentRadius;
    float subMarineCohesionRadius;

    float subMarineSeparationEulerAngle;
    float subMarineAlignmentEulerAngle;
    float subMarineCohesionEulerAngle;

    float subMarineSeparationCos;
    float subMarineAlignmentCos;
    float subMarineCohesionCos;

    float subMarineSeparationConstant;
    float subMarineAlignmentConstant;
    float subMarineCohesionConstant;

    int placeholder0;
    int placeholder1;
};

#endif
