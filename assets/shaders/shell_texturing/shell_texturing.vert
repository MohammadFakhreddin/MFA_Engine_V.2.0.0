#include "../math_common.glsl"

// Per vertex
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inCenter;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outWorldNormal;
layout(location = 3) out vec3 outWorldCenter;
layout(location = 4) out float outVertexIndex;
layout(location = 5) flat out uint outInstanceIndex;

layout(set = 0, binding = 0, std140) uniform CameraBuffer
{
    mat4 viewProjection;
} camera;

layout(set = 1, binding = 0, std140) uniform ShellBuffer
{
    float displacement;                                 // The distance between each layer
    float radius;                                       // For now I assume we have same radius in entire model.
    int count;                                          // The number of shells layer we render.
    float placeholder0;
} shell;

layout(set = 2, binding = 0, std140) uniform ModelBuffer
{
    mat4 model;
};

void main()
{
    uint shellIndex = uint(gl_InstanceIndex);

    // We remove inverse transpose of model matrix for normal calculation to compensate for non-uniform scaling. 
    vec3 worldNormal = normalize(transpose(inverse(mat3(model))) * inNormal);

    vec3 displacement = worldNormal * shell.displacement * shellIndex;
    vec4 worldPosition = model * vec4(inPosition + displacement, 1.0);
    vec4 worldCenter = model * vec4(inCenter + displacement, 1.0);

    gl_Position = camera.viewProjection * worldPosition;

    outWorldPosition = worldPosition.xyz;
    outUV = inUV;
    outWorldNormal = worldNormal;
    outWorldCenter = worldCenter.xyz;

    outVertexIndex = float(gl_VertexIndex);
    outInstanceIndex = uint(gl_InstanceIndex);
}
