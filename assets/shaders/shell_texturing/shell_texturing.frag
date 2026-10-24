#include "../math_common.glsl"

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inWorldNormal;
layout(location = 3) flat in float inVertexIndex;
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

    vec4 albedoColor = material.albedo;

    vec3 rgbColor = albedoColor.rgb;

    float alpha = albedoColor.a;

    // We skip this operation for shell 0
    if (shellIndex > 0)
    {
        
        float shellFraction = float(shellIndex) / float(max(shellCount, 1));

        vec2 uv = inUV * shell.density;

        float uvFracX = fract(uv.x);
        float uvFracY = fract(uv.y);
        
        float uvIntX = uv.x - uvFracX;
        float uvIntY = uv.y - uvFracY;

        float seed = uvIntX + 100.0 * uvIntY + 100.0 * 10.0;

        uvFracX = uvFracX * 2.0 - 1.0;
        uvFracY = uvFracY * 2.0 - 1.0;
        
        float distance = length(vec2(uvFracX, uvFracY));
        if (distance > shell.radius * (rand01(seed) - shellFraction))
        {
            discard;
        }
    }

    float nDotL = dot(normalize(light.direction), normalize(inWorldNormal)) * 0.5 + 0.5;
    
    float halfLambert = nDotL * nDotL;
    
    float attenuation = float(shellIndex) / float(max(shellCount - 1, 1));

    rgbColor = (halfLambert * light.color * attenuation + light.ambientStrength) * rgbColor;

    outColor = vec4(rgbColor, alpha);
}
