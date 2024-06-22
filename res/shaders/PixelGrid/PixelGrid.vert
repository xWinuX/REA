#version 450

#include "../globals.glsl"

layout(location = 0) in uint a_Index;

layout(location = 0) out vec2 v_PixelPosition;

const vec2 POSITION[4] = vec2[4] (
    vec2(-0.5f, 0.5),
    vec2(0.5f, 0.5f),
    vec2(-0.5f, -0.5f),
    vec2(0.5f, -0.5f)
);

const vec2 UV[4] = vec2[4] (
    vec2(0.0f, 1.0f), // Top Left
    vec2(1.0f, 1.0f), // Top  Right
    vec2(0.0f, 0.0f), // Bottom Left
    vec2(1.0f, 0.0f)  // Bottom Right
);

layout(std430, set = 1, binding = 0) readonly buffer c_GridInfo {
    int width;
    int height;
    float zoom;
    uint renderMode;
    vec2 offset;
    vec2 pointerPosition;
    vec4 colorLookup[256];
} gridInfo;

void main() {

    vec2 gridSize = vec2(gridInfo.width, gridInfo.height);

    gl_Position = cameraProperties.proj * cameraProperties.view * vec4((POSITION[a_Index]*gridSize) / cameraProperties.pixelsPerUnit, -100.0f, 1.0f);

    v_PixelPosition = UV[a_Index] * gridSize;
}