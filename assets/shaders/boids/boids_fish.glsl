#ifndef BOIDS_FISH
#define BOIDS_FISH

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
};

#endif
