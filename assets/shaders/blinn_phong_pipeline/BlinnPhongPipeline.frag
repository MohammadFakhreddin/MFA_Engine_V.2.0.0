#version 450

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in float inSpecularStrength;
layout(location = 4) flat in int inShininess;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, std140) uniform CameraBuffer
{
    mat4 viewProjection;
    vec3 position;
    float placeholder;
} camera;

layout(set = 0, binding = 1, std140) uniform LightSourceBuffer
{
    vec3 direction;
    float ambientStrength;
    vec3 color;
    float placeholder1;
} light;

void main()
{
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
        vec3 specularStrength = vec3(inSpecularStrength);
        specular = specularStrength
            * pow(max(dot(viewDir, reflectDir), 0.0), float(inShininess))
            * lightColor;
    }

    outColor = vec4(ambient + diffuse + specular, 1.0) * inColor;
}
