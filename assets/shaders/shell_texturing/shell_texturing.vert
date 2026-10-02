#include "../math_common.glsl"

// Per vertex
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outWorldNormal;
layout(location = 3) out float outVertexIndex;
layout(location = 4) flat out uint outInstanceIndex;

layout(set = 0, binding = 0, std140) uniform CameraBuffer
{
    mat4 viewProjection;
} camera;

layout(set = 1, binding = 0, std140) uniform ShellBuffer
{
    float displacement;                                 // The distance between each layer
    float radius;                                       // For now I assume we have same radius in entire model.
    int count;                                          // The number of shells layer we render.
    float attenuation;                                  // Introduces exponential displacement so earlier shells are closer to the original mesh and later shells are further away.

    int density;                                        // The density of the shells. This is used to determine how many shells to render based on the distance from the camera.
    vec3 placeholder;                                   // Placeholder to ensure the struct is aligned to 16 bytes.
} shell;

layout(set = 2, binding = 0, std140) uniform ModelBuffer
{
    mat4 model;
};

void main()
{
    uint shellIndex = uint(gl_InstanceIndex);
    float shellHeight = float(shellIndex) / float(shell.count);

    // We remove inverse transpose of model matrix for normal calculation to compensate for non-uniform scaling. 
    vec3 worldNormal = normalize(transpose(inverse(mat3(model))) * inNormal);

    vec3 displacement = worldNormal * shell.displacement * pow(shellHeight, shell.attenuation);
    vec4 worldPosition = model * vec4(inPosition + displacement, 1.0);

    gl_Position = camera.viewProjection * worldPosition;

    outWorldPosition = worldPosition.xyz;
    outUV = inUV;
    outWorldNormal = normalize(mat3(camera.viewProjection) * worldNormal);

    outVertexIndex = float(gl_VertexIndex);
    outInstanceIndex = uint(gl_InstanceIndex);
}
