#version 450

#include "../PixelGridGlobals.glsl"

layout (location = 0) in vec2 v_PixelPosition;

layout (location = 0) out vec4 outColor;

layout (std430, set = 1, binding = 0) readonly buffer c_GridInfo {
    int width;
    int height;
    float zoom;
    uint renderMode;
    vec2 offset;
    vec2 pointerPosition;
    vec4 colorLookup[256];
} gridInfo;

layout (set = 1, binding = 1) uniform sampler2D texSampler;

layout (std430, set = 2, binding = 0) buffer s_dl_ViewportPixels {
    Pixel viewportPixels[NumSimulatedPixels];
};

void main() {
    ivec2 position = ivec2(floor(v_PixelPosition));

    vec4 pixelColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    int height = gridInfo.height;
    int index = (position.y * gridInfo.width) + position.x;
    Pixel pixel = viewportPixels[index];
    uint pixelID = getPixelID(pixel.PixelID16_Charge8_Flags8);
    uint charge = getCharge(pixel.PixelID16_Charge8_Flags8);
    float temperature = pixel.Temperature;

    pixelColor = gridInfo.colorLookup[pixelID];

    float temperatureScaled = temperature / 255.0f;
    if (gridInfo.renderMode == RenderMode_Normal) {
        pixelColor *= 1 + vec4(temperature / 100.0f, temperatureScaled, temperatureScaled, 1.0f);
        pixelColor += vec4((charge / 255.0f) * 2.0f, (charge / 255.0f) * 0.2, 0.0f, 0.0f);

    /*
        //            pixelColor = vec4(dirColor.xyz, 1.0f);
        int label = labels[index];

        pixelColor = vec4(0.0f, label/1048576.0f, pixelColor.z, 1.0f);*/

    }
    if (gridInfo.renderMode == RenderMode_Temperature) {
        pixelColor = vec4(temperature / 255.0f, 0.0f, 0.0f, 1.0f);
    }

    uvec2 offset = uvec2(gridInfo.offset);
    bool isCursorPixel = int(gridInfo.pointerPosition.x - offset.x) == position.x && int(gridInfo.pointerPosition.y - offset.y) == position.y;

    outColor = (isCursorPixel ? vec4(1.0f) : pixelColor);

    //outColor = vec4(label/1000000.0f, 0.0f, 0.0f, 1.0f);
}