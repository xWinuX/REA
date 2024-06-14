#version 450

#include "../PixelGridGlobals.glsl"

layout(location = 0) in vec2 v_PixelPosition;

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
    Pixel pixels[NumPixels];
};


const vec3 POSITION[4] = vec3[4] (
    vec3(1.0f, 0.0f, 0.0f),  // Right
    vec3(0.0f, 1.0f, 0.0f),  // Up
    vec3(0.0f, 0.0f, 1.0f),  // Left
    vec3(1.0f, 1.0f, 0.0f)   // Down
);

void main() {
    ivec2 position = ivec2(floor(v_PixelPosition));

    vec4 pixelColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    if (position.x >= 0 && position.x < gridInfo.width && position.y >= 0 && position.y < gridInfo.height)
    {
        int height = gridInfo.height;
        int index = (position.y * gridInfo.width) + position.x;
        Pixel pixel = pixels[index];
        uint pixelID = getPixelID(pixel.PixelID16_Temperature8_Pressure8);
        uint charge = getCharge(pixel.PixelID16_Temperature8_Pressure8);
        uint dir = getDirection(pixel.PixelID16_Temperature8_Pressure8);
        float temperature = pixel.Temperature;

        pixelColor = gridInfo.colorLookup[pixelID];


        vec4 dirColor = vec4((dir >> 0u)  & 1u, (dir >> 1u) & 1u, (dir >> 2u) & 1u, (dir >> 3u) & 1u);
        float temperatureScaled = temperature/255.0f;
        if (gridInfo.renderMode == RenderMode_Normal) {
            //pixelColor *= 1 + vec4(temperature/100.0f, temperatureScaled, temperatureScaled, 1.0f);
            pixelColor = vec4(charge/255.0f, pixelColor.y, pixelColor.z, 1.0f);

//            pixelColor = vec4(dirColor.xyz, 1.0f);
        }
        if (gridInfo.renderMode == RenderMode_Temperature) {
        //    pixelColor = vec4(temperature/255.0f, 0.0f, 0.0f, 1.0f);
            pixelColor = vec4(dirColor.xyz + (0.5f * dirColor.w), 1.0f);
        }
    }

    bool isCursorPixel = int(gridInfo.pointerPosition.x) == position.x && int(gridInfo.pointerPosition.y) == position.y;

    outColor = (isCursorPixel ? vec4(1.0f) : pixelColor);
}