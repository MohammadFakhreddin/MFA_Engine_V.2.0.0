#version 450

#ifndef BOIDS_COMMON
#define BOIDS_COMMON

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

struct Fish
{
    int id;                         // Fish unique index, starts from 0

    vec3 position;

    vec3 velocity;
    int placeholder0;

    vec3 force;
    int placeholder1;

    vec3 scale;
    int placeholder2;

    mat4 localTransform;            // The default transform used to adjust the mesh

    vec4 previousRotatonQuat;       // Used for animation
};

struct CollisionTriangle
{
    vec3 v0;
    int placeholder0;

    vec3 v1;
    int placeholder1;

    vec3 v2;
    int placeholder2;

    vec3 normal;
    int placeholder3;
};

#endif