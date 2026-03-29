layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 3) in vec4 inModelCol0;
layout(location = 4) in vec4 inModelCol1;
layout(location = 5) in vec4 inModelCol2;
layout(location = 6) in vec4 inModelCol3;

layout(location = 7) in int inMaterialId;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) flat out int outMaterialId;

layout(set = 0, binding = 0, std140) uniform CameraBuffer
{
    mat4 viewProjection;
    vec3 position;
    float placeholder;
} camera;

void main()
{
    mat4 model = mat4(
        inModelCol0,
        inModelCol1,
        inModelCol2,
        inModelCol3
    );
    
    vec4 worldPosition = model * vec4(inPosition, 1.0);

    mat3 normalMatrix = transpose(inverse(mat3(model)));

    outWorldPosition = worldPosition.xyz;
    outWorldNormal = normalMatrix * inNormal;
    outUV = inUV;

    gl_Position = camera.viewProjection * worldPosition;

    outMaterialId = inMaterialId;
}
