#ifndef BOIDS_FISH
#define BOIDS_FISH

struct Fish
{
    int id;                         // Fish unique index, starts from 0
    int pad0;
    int pad1;
    int pad2;

    vec3 rbPosition;
    int pad3;
    
    vec3 rbVelocity;
    int pad4;

    vec3 rbForce;
    int pad5;

    vec3 tPosition;
    int pad6;

    vec4 tRotation;                 // Used for animation

    vec3 tScale;
    int pad7;

    mat4 tLocalMat4;                // The default transform used to adjust the mesh
};

#endif
