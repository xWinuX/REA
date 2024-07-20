#include "PixelGridGlobals.glsl"

layout(std430, set = 1, binding = 0) buffer c_s_si_SimulationData {
    float deltaTime;
    uint timer;
    float rng;
    uint width;
    uint height;
    uint simulationWidth;
    uint simulationHeight;
    vec2 targetPosition;
    PixelData pixelLookup[1024];
} simulationData;

layout(std430, set = 1, binding = 1)  buffer s_dl_ViewportPixels {
    Pixel readOnlyPixels[NumSimulatedPixels];
};

layout(std430, set = 1, binding = 2)  buffer s_si_WorldPixels {
    Pixel pixels[NumPixels];
};

layout(std430, set = 1, binding = 3) buffer s_si_RigidBodyData {
    RigidBody rigidBodies[NumRigidbodies];
};

layout(std430, set = 1, binding = 4)  buffer s_si_dl_RigidBodyPixelData {
    Pixel rigidBodyData[NumSimulatedPixels];
};
