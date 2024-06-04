#version 450

#include "../PixelGridGlobals.glsl"

layout(location = 0) in vec2 fragPosition;
layout(location = 0) out vec4 outColor;

layout(std430, set = 1, binding = 0) readonly buffer c_GridInfo {
    int width;
    int height;
    float zoom;
    uint renderMode;
    vec2 offset;
    vec2 pointerPosition;
    vec4 colorLookup[256];
} gridInfo;

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(std430, set = 2, binding = 0) buffer s_Pixels {
    Pixel pixels[1000000];
};

void main() {
    int height = gridInfo.height;
    int index = (int(fragPosition.y) * gridInfo.width + int(fragPosition.x));
    Pixel pixel = pixels[index];
    uint pixelID = getPixelID(pixel.PixelID16_Temperature8_Pressure8);
    float temperature = pixel.Temperature;

    vec4 pixelColor = gridInfo.colorLookup[pixelID];

    float temperatureScaled = temperature/255.0f;
    if (gridInfo.renderMode == RenderMode_Normal) { pixelColor *= 1 + vec4(temperature/30.0f, temperatureScaled, temperatureScaled, 1.0f); }
    if (gridInfo.renderMode == RenderMode_Temperature) { pixelColor = vec4(temperature/255.0f, 0.0f, 0.0f, 1.0f); }

    bool isCursorPixel = int(gridInfo.pointerPosition.x) == int(fragPosition.x) && int(gridInfo.pointerPosition.y) == int(fragPosition.y);

    outColor = isCursorPixel ? vec4(1.0f) : pixelColor;
}