#version 450

layout(location = 0) in vec2 fragPosition;

layout(location = 0) out vec4 outColor;

layout(std430, set = 1, binding = 0) readonly buffer si_GridInfo {
    int width;
    int height;
    vec4 colorLookup[16];
} gridInfo;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(std430, set = 2, binding = 0) readonly buffer GridData {
    uint tileIDs[1000000];
} gridData;

void main() {
    int height = gridInfo.height;
    int index = (int(fragPosition.y) * gridInfo.width + int(fragPosition.x));
    outColor = gridInfo.colorLookup[gridData.tileIDs[index]];
}