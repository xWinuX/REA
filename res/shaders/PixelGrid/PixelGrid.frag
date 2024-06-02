#version 450

layout(location = 0) in vec2 fragPosition;

layout(location = 0) out vec4 outColor;

layout(std140, set = 1, binding = 0) readonly buffer c_GridInfo {
    int width;
    int height;
    float zoom;
    vec2 offset;
    vec2 pointerPosition;
    vec4 colorLookup[256];
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
    uint pixelID = pixels[index].PixelID8_Flags8_Density8_Spread8;

  /*  uint density = (pixels[index].PixelID8_Flags8_Density8_Spread8 >> 16u) & 0xFFu;
     uint flags = (pixels[index].PixelID8_Flags8_Density8_Spread8 >> 8u) & 0xFFu;
     uint spread = (pixels[index].PixelID8_Flags8_Density8_Spread8 >> 24u) & 0xFFu;
     outColor = vec4(density/100.0f, flags/100.0f, 0.0f, 1.0f);
*/
    vec4 pixelColor = gridInfo.colorLookup[pixelID & 0xFFFFu];
    uint temperature = (pixels[index].PixelID8_Flags8_Density8_Spread8 >> 16u) & 0xFFu;
    bool isCursorPixel = int(gridInfo.pointerPosition.x) == int(fragPosition.x) && int(gridInfo.pointerPosition.y) == int(fragPosition.y);
    outColor = isCursorPixel ? vec4(1.0f) : pixelColor * (1.0f + (temperature/255.0f));//;
    //outColor = vec4(vec3(pixelID /10.f), 1.0f);
}