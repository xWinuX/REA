#version 450

layout(location = 0) in vec2 fragPosition;

layout(location = 0) out vec4 outColor;

layout(std430, set = 1, binding = 0) readonly buffer si_TileData {
    vec4 colorLookup[16];
} tileData;

layout(std430, set = 2, binding = 0) readonly buffer si_GridBuffer {
    uint tileIDs[10000];
} gridBuffer;

void main() {
    int width = 100;
    int height = 100;
    int index = int(fragPosition.x * width) + (int(fragPosition.y * height) * height);
    outColor = tileData.colorLookup[gridBuffer.tileIDs[index]];
}