#ifndef MATH_COMMON
#define MATH_COMMON

#define FORWARD_VEC3 vec3(0.0f, 0.0f, 1.0f)
#define RIGHT_VEC3 vec3(1.0f, 0.0f, 0.0f);
#define UP_VEC3 vec3(0.0f, 1.0f, 0.0f);

#define EPSILON 1e-6f

bool Math_IsInsideTriangleBarycentric(vec3 q, vec3 p0, vec3 p1, vec3 p2)
{
    vec3 v0 = p0 - p2;
    vec3 v1 = p1 - p2;
    vec3 v2 = q  - p2;

    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d02 = dot(v0, v2);
    float d11 = dot(v1, v1);
    float d12 = dot(v1, v2);

    float det = (d11 * d00) - (d01 * d01);

    // Degenerate triangle
    if (abs(det) < EPSILON)
    {
        return false;
    }

    float a = ((d11 * d02) - (d01 * d12)) / det;
    float b = ((d12 * d00) - (d01 * d02)) / det;

    return (a >= 0.0) && (b >= 0.0) && ((a + b) <= 1.0);
}

#endif