#include "../math_common.glsl"

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inWorldNormal;
layout(location = 3) in float inVertexIndex;
layout(location = 4) flat in uint inInstanceIndex;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0, std140) uniform ShellBuffer
{
    float displacement;                                 // The distance between each layer
    float radius;                                       // For now I assume we have same radius in entire model.
    int count;                                          // The number of shells layer we render.
    float attenuation;

    int density;
    vec3 placeholder;
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
    float placeholder;
} light;

void main()
{
    uint shellIndex = uint(inInstanceIndex);

    uint shellCount = uint(shell.count);

    // We skip this operation for shell 0
    if (shellIndex > 0)
    {
        float shellFraction = float(shellIndex) / float(max(shellCount - 1, 1));

        vec2 uv = inUV * shell.density;
        vec2 uvFrac = fract(uv) * 2.0 - 1.0;
        if (length(uvFrac) > shell.radius * (rand01(inVertexIndex) - shellFraction))
        {
            discard;
        }
    }

    vec4 color = material.albedo;

    if (material.albedoTexture >= 0)
    {
        color *= texture(albedoTexture, inUV);
    }

    float heightAttenuation = (float(shellIndex + 1) / float(max(shellCount, 1)));

    vec3 rgbColor = color.rgb;
    float alpha = color.a;

    float nDotL = clamp(dot(normalize(light.direction), normalize(inWorldNormal)), 0.0, 1.0) * 0.5 + 0.5;
    float halfLambert = nDotL * nDotL;
    
    float attenuation = float(shellIndex) / float(max(shellCount - 1, 1));

    rgbColor = (halfLambert * light.color * attenuation + light.ambientStrength) * rgbColor;

    outColor = vec4(rgbColor, alpha);
}
