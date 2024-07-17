#include "PixelGridGlobals.glsl"

layout(std430, set = 1, binding = 0) buffer c_s_si_SimulationData {
    float deltaTime;
    uint timer;
    float rng;
    uint width;
    uint height;
    PixelData pixelLookup[1024];
} simulationData;

layout(std430, set = 1, binding = 1)  buffer s_PixelSSBOIn {
    Pixel readOnlyPixels[NumPixels];
};

layout(std430, set = 1, binding = 2)  buffer s_si_Pixels {
    Pixel pixels[NumPixels*2];
};

layout(std430, set = 1, binding = 3) buffer s_si_RigidBodyData {
    RigidBody rigidBodies[NumRigidbodies];
};

layout(std430, set = 1, binding = 4)  buffer s_si_dl_RigidBodyPixelData {
    Pixel rigidBodyData[NumPixels];
};
