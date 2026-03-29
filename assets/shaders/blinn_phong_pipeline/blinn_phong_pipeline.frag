layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) flat in int inMaterialId;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, std140) uniform CameraBuffer
{
    mat4 viewProjection;
    vec3 position;
    float placeholder;
} camera;

layout(set = 1, binding = 0, std140) uniform LightSourceBuffer
{
    vec3 direction;
    float ambientStrength;
    vec3 color;
    float placeholder1;
} light;

// TODO: This material can support many kinds of rendering similar to uber shaders and we can generalize this file in future but not today.
struct Material
{
    vec4 albedo;
    
    float specularStrength;
    float shininess;
    int albedoTexture;
    int enableLighting;
};

layout(set = 2, binding = 0) readonly buffer MaterialBuffer
{
    Material materials[];
};

layout(set = 2, binding = 1) uniform sampler2D textures[8];

#define material materials[inMaterialId]

void main()
{
    vec4 albedo = material.albedo.rgba;
    int albedoTexture = material.albedoTexture;
    if (albedoTexture >= 0)
    {
        albedo *= texture(textures[albedoTexture], inUV).rgba;
    }

    if (material.enableLighting == 1)
    {
        float specularStrength = material.specularStrength;
        float shininess = material.shininess;
        
        vec3 fragNormal = normalize(inWorldNormal);
        vec3 lightDir = -normalize(light.direction);
        vec3 lightColor = light.color;

        float diffuseDot = dot(lightDir, fragNormal);
        vec3 ambient = light.ambientStrength * lightColor;
        vec3 diffuse = max(diffuseDot, 0.0) * lightColor;

        vec3 viewDir = normalize(camera.position - inWorldPosition);
        vec3 reflectDir = reflect(-lightDir, fragNormal);

        vec3 specular = vec3(0.0);
        if (diffuseDot > 0.0)
        {
            specular = specularStrength
                * pow(max(dot(viewDir, reflectDir), 0.0), float(shininess))
                * lightColor;
        }
        
        outColor = vec4(ambient + diffuse + specular, 1.0) * albedo;
    }
    else
    {
        outColor = albedo;
    }
}
