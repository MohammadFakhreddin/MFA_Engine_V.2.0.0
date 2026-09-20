#include "../math_common.glsl"

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inWorldNormal;
layout(location = 3) in vec3 inWorldCenter;
layout(location = 4) in float inVertexIndex;
layout(location = 5) flat in uint inInstanceIndex;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0, std140) uniform ShellBuffer
{
    float displacement;                                 // The distance between each layer
    float radius;                                       // For now I assume we have same radius in entire model.
    int count;                                          // The number of shells layer we render.
    float placeholder0;
} shell;

layout(set = 3, binding = 0) uniform MaterialBuffer
{
    vec4 albedo;

    int albedoTexture;
    vec3 placeholder0;
} material;

layout(set = 4, binding = 0) uniform sampler2D albedoTexture;

layout(set = 5, binding = 0, std140) uniform LightBuffer
{
    vec3 direction;
    float ambientStrength;
    vec3 color;
    float heightAttenuation;
} light;

void main()
{
    uint shellIndex = uint(inInstanceIndex);
    uint shellCount = uint(shell.count);
    float shellFraction = float(shellIndex) / float(max(shellCount - 1, 1));

    float radius = length(inWorldPosition - inWorldCenter);
    if (radius > shell.radius * (rand01(inVertexIndex) - shellFraction))
    {
        discard;
    }

    vec4 color = material.albedo;
    if (material.albedoTexture >= 0)
    {
        color *= texture(albedoTexture, inUV);
    }

    vec3 ambient = light.ambientStrength * light.color;
    
    float halfLambert = dot(normalize(light.direction), normalize(inWorldNormal)) * 0.5 + 0.5;
    vec3 diffuse = halfLambert * light.color;

    float heightAttenuation = shellFraction;

    outColor = vec4(heightAttenuation * (diffuse + ambient) * color.rgb, color.a);
}
