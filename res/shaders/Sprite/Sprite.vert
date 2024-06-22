#version 450

layout(location = 0) in uint inIndex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out int textureIndex;

#include "../globals.glsl"

struct TextureData {
    vec3 pageIndexAndSize;
    vec4[4] uvs;
};

layout(std140, set = 1, binding = 0) readonly buffer si_TextureStore {
    TextureData textures[10240];
} textureStore;

layout(std430, set = 2, binding = 0) readonly buffer ObjectBuffer {
    vec4 positions[2048000];
    uint textureIDs[2048000];
    uint numObjects;
} objectBuffer;

const vec2 POSITIONS[4] = vec2[4] (
    vec2(-0.5f, 0.5),
    vec2(0.5f, 0.5f),
    vec2(-0.5f, -0.5f),
    vec2(0.5f, -0.5f)
);

void main() {
    int i = gl_InstanceIndex*10240 + (gl_VertexIndex / 6);
    vec3 position = objectBuffer.positions[i].xyz;
    TextureData textureData = textureStore.textures[objectBuffer.textureIDs[i]];

    float angle = radians(objectBuffer.positions[i].w);

    mat2 rotationMatrix = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));

    float pixelScaling = cameraProperties.pixelsPerUnit / cameraProperties.pixelSize;
    vec2 scaledPosition = (POSITIONS[inIndex] * vec2(textureData.pageIndexAndSize.y, textureData.pageIndexAndSize.z)) / (cameraProperties.pixelsPerUnit);
    vec2 rotatedPosition = rotationMatrix * scaledPosition;

    vec3 adjustedPosition = vec3(rotatedPosition + position.xy, position.z);

    gl_Position = cameraProperties.proj * cameraProperties.view * vec4(adjustedPosition, 1.0);
    fragColor = vec4(1.0f, 1.0f, 1.0f, float(i < objectBuffer.numObjects));
    fragTexCoord = textureData.uvs[inIndex].xy;
    textureIndex = int(textureData.pageIndexAndSize.x);
}