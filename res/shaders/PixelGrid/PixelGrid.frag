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

layout(set = 1, binding = 1) buffer s_si_dl_Labels {
    int labels[NumPixels];
};


layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(std430, set = 2, binding = 0) buffer s_PixelSSBOIn {
    Pixel readOnlyPixels[NumPixels];
};

void main() {
    ivec2 position = ivec2(floor(v_PixelPosition));

    vec4 pixelColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    int height = gridInfo.height;
    int index = (position.y * gridInfo.width) + position.x;
    Pixel pixel = readOnlyPixels[index];
    uint pixelID = getPixelID(pixel.PixelID16_Charge8_Flags8);
    uint charge = getCharge(pixel.PixelID16_Charge8_Flags8);
    float temperature = pixel.Temperature;

    pixelColor = gridInfo.colorLookup[pixelID];

    float temperatureScaled = temperature/255.0f;
    if (gridInfo.renderMode == RenderMode_Normal) {
        pixelColor *= 1 + vec4(temperature/100.0f, temperatureScaled, temperatureScaled, 1.0f);

        /*
        //            pixelColor = vec4(dirColor.xyz, 1.0f);
        int label = labels[index];

        pixelColor = vec4(0.0f, label/1048576.0f, pixelColor.z, 1.0f);*/

    }
    if (gridInfo.renderMode == RenderMode_Temperature) {
        //    pixelColor = vec4(temperature/255.0f, 0.0f, 0.0f, 1.0f);
        //pixelColor = vec4(dirColor.xyz + (0.5f * dirColor.w), 1.0f);

        uint id = getRigidBodyID(pixel.RigidBodyID12_RigidBodyIndex20);


        pixelColor = vec4(id/4.0f, 0.0f, 0.0f, 1.0f);
    }

    bool isCursorPixel = int(gridInfo.pointerPosition.x) == position.x && int(gridInfo.pointerPosition.y) == position.y;

    outColor = (isCursorPixel ? vec4(1.0f) : pixelColor);

    //outColor = vec4(label/1000000.0f, 0.0f, 0.0f, 1.0f);
}