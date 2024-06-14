#version 450

#include "../globals.glsl"

layout(location = 0) in uint a_Index;

layout(location = 0) out vec2 v_PixelPosition;

const vec2 POSITION[4] = vec2[4] (
    vec2(-1.0f, 1.0f),  // Top Left
    vec2(1.0f, 1.0f),   // Top  Right
    vec2(-1.0f, -1.0f), // Bottom Left
    vec2(1.0f, -1.0f)   // Bottom Right
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
    gl_Position = vec4(POSITION[a_Index], 0.0f, 1.0f);

    v_PixelPosition = (UV[a_Index] * (vec2(gridInfo.width, gridInfo.height) / gridInfo.zoom)) + gridInfo.offset;
}