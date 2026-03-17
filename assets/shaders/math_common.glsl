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

float Math_ACosSafe(float value)
{
    return acos(clamp(value, -1.0, 1.0));
}

mat4 Math_ChangeOfBasis(vec3 biTangent, vec3 normal, vec3 tangent)
{
    return mat4(
        vec4(biTangent, 0.0),
        vec4(normal, 0.0),
        vec4(tangent, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
}

vec4 Math_QuatFromMat3(mat3 m)
{
    float m00 = m[0][0];
    float m01 = m[1][0];
    float m02 = m[2][0];
    float m10 = m[0][1];
    float m11 = m[1][1];
    float m12 = m[2][1];
    float m20 = m[0][2];
    float m21 = m[1][2];
    float m22 = m[2][2];

    float trace = m00 + m11 + m22;
    vec4 q;

    if (trace > 0.0)
    {
        float s = sqrt(trace + 1.0) * 2.0;
        q = vec4(
            (m21 - m12) / s,
            (m02 - m20) / s,
            (m10 - m01) / s,
            0.25 * s
        );
    }
    else if (m00 > m11 && m00 > m22)
    {
        float s = sqrt(1.0 + m00 - m11 - m22) * 2.0;
        q = vec4(
            0.25 * s,
            (m01 + m10) / s,
            (m02 + m20) / s,
            (m21 - m12) / s
        );
    }
    else if (m11 > m22)
    {
        float s = sqrt(1.0 + m11 - m00 - m22) * 2.0;
        q = vec4(
            (m01 + m10) / s,
            0.25 * s,
            (m12 + m21) / s,
            (m02 - m20) / s
        );
    }
    else
    {
        float s = sqrt(1.0 + m22 - m00 - m11) * 2.0;
        q = vec4(
            (m02 + m20) / s,
            (m12 + m21) / s,
            0.25 * s,
            (m10 - m01) / s
        );
    }

    return normalize(q);
}

vec4 Math_RotateTowards(vec4 from, vec4 to, float maxAngleDelta)
{
    vec4 fromN = normalize(from);
    vec4 toN = normalize(to);

    float cosTheta = dot(fromN, toN);
    if (cosTheta >= 1.0 - EPSILON)
    {
        return toN;
    }

    if (cosTheta < 0.0)
    {
        toN = -toN;
        cosTheta = -cosTheta;
    }

    float halfTheta = Math_ACosSafe(cosTheta);
    float theta = halfTheta * 2.0;
    float fraction = clamp(maxAngleDelta / max(theta, EPSILON), 0.0, 1.0);

    if (cosTheta > 1.0 - EPSILON)
    {
        return normalize(mix(fromN, toN, fraction));
    }

    float sinHalfTheta = sin(halfTheta);
    return normalize(
        (sin((1.0 - fraction) * halfTheta) * fromN + sin(fraction * halfTheta) * toN)
        / max(sinHalfTheta, EPSILON)
    );
}

mat4 Math_TranslateToMat4(vec3 translation)
{
    mat4 result = mat4(1.0);
    result[3] = vec4(translation, 1.0);
    return result;
}

mat4 Math_ScaleToMat4(vec3 scale)
{
    mat4 result = mat4(1.0);
    result[0][0] = scale.x;
    result[1][1] = scale.y;
    result[2][2] = scale.z;
    return result;
}

mat4 Math_QuatToMat4(vec4 quat)
{
    vec4 q = normalize(quat);

    float x = q.x;
    float y = q.y;
    float z = q.z;
    float w = q.w;

    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float xy = x * y;
    float xz = x * z;
    float yz = y * z;
    float wx = w * x;
    float wy = w * y;
    float wz = w * z;

    mat4 result = mat4(1.0);

    result[0][0] = 1.0 - 2.0 * (yy + zz);
    result[0][1] = 2.0 * (xy + wz);
    result[0][2] = 2.0 * (xz - wy);

    result[1][0] = 2.0 * (xy - wz);
    result[1][1] = 1.0 - 2.0 * (xx + zz);
    result[1][2] = 2.0 * (yz + wx);

    result[2][0] = 2.0 * (xz + wy);
    result[2][1] = 2.0 * (yz - wx);
    result[2][2] = 1.0 - 2.0 * (xx + yy);

    return result;
}

#endif