#version 450

layout(location = 0) in uint inIndex;

layout(location = 0) out vec2 fragPosition;

#include "../globals.glsl"

const vec2 POSITIONS[4] = vec2[4] (
    vec2(0.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(0.0f, 0.0f),
    vec2(1.0f, 0.0f)
);

const vec2 POSITIONS_NDC[4] = vec2[4] (
    vec2(-1.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(-1.0f, -1.0f),
    vec2(1.0f, -1.0f)
);

layout(std140, set = 1, binding = 0) readonly buffer c_GridInfo {
    int width;
    int height;
    float zoom;
    vec2 offset;
    vec2 pointerPosition;
    vec4 colorLookup[256];
} gridInfo;

void main() {
    float height = gridInfo.height;
    float width = float(gridInfo.width) / float(height);

    gl_Position =  vec4(POSITIONS_NDC[inIndex], 0.0f, 1.0f);

    //vec2 position = (POSITIONS[inIndex] * (vec2(gridInfo.width, gridInfo.height) / gridInfo.zoom)) + gridInfo.offset;
    //fragPosition = vec2(clamp(position.x, 0, gridInfo.width), clamp(position.y, 0, gridInfo.height));

    fragPosition = (POSITIONS[inIndex] * (vec2(gridInfo.width, gridInfo.height) / gridInfo.zoom)) + gridInfo.offset;

}