#version 450

layout(location = 0) in vec2 fragPosition;

layout(location = 0) out vec4 outColor;

layout(std140, set = 1, binding = 0) readonly buffer si_GridInfo {
    int width;
    int height;
    float zoom;
    vec2 offset;
    vec4 colorLookup[16];
} gridInfo;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

struct Pixel {
    uint PixelID8_Flags8_Density8_Spread8;
};

layout(std430, set = 2, binding = 0) buffer s_Pixels {
    Pixel pixels[1000000];
};

void main() {
    int height = gridInfo.height;
    int index = (int(fragPosition.y) * gridInfo.width + int(fragPosition.x));
    uint pixelID = pixels[index].PixelID8_Flags8_Density8_Spread8 & 0xFFu;
    outColor = gridInfo.colorLookup[pixelID];
}