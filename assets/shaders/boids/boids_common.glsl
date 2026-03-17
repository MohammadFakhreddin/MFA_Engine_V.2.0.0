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

    vec3 rbPosition;

    vec3 rbVelocity;
    int placeholder0;

    vec3 rbForce;
    int placeholder1;

    vec3 tPosition;
    int placeholder2;

    vec4 tRotation;                 // Used for animation

    vec3 tScale;
    int placeholder3;

    mat4 tLocalMat4;                // The default transform used to adjust the mesh

    mat4 tGlobalMat4;               // The global transform used for rendering
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