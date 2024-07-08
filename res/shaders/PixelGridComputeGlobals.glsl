#include "PixelGridGlobals.glsl"

layout(std430, set = 1, binding = 0) readonly buffer c_s_si_SimulationData {
    float deltaTime;
    uint timer;
    float rng;
    uint width;
    uint height;
    PixelData pixelLookup[1024];
    RigidBody rigidBodies[NumRigidbodies];
} simulationData;

layout(std430, set = 1, binding = 1)  buffer na_s_PixelSSBOIn {
    Pixel readOnlyPixels[NumPixels];
};


layout(std430, set = 1, binding = 2)  buffer s_Pixels {
    Pixel pixels[NumPixels];
};


layout(std430, set = 1, binding = 3)  buffer s_si_dl_RigidBodyData {
    Pixel rigidBodyData[NumPixels];
};
