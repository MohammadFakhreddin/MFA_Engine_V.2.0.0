layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inModelRow0;
layout(location = 3) in vec4 inModelRow1;
layout(location = 4) in vec4 inModelRow2;
layout(location = 5) in vec4 inModelRow3;
layout(location = 6) in vec4 inColor;
layout(location = 7) in float inSpecularStrength;
layout(location = 8) in int inShininess;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec4 outColor;
layout(location = 3) out float outSpecularStrength;
layout(location = 4) flat out int outShininess;

layout(set = 0, binding = 0, std140) uniform CameraBuffer
{
    mat4 viewProjection;
    vec3 position;
    float placeholder;
} camera;

void main()
{
    mat4 model = transpose(mat4(
        inModelRow0,
        inModelRow1,
        inModelRow2,
        inModelRow3
    ));

    vec4 worldPosition = model * vec4(inPosition, 1.0);
    outWorldPosition = worldPosition.xyz;
    outWorldNormal = (model * vec4(inNormal, 0.0)).xyz;

    gl_Position = camera.viewProjection * worldPosition;

    outColor = inColor;
    outSpecularStrength = inSpecularStrength;
    outShininess = inShininess;
}
