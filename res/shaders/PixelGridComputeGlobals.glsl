

struct Pixel {
    uint PixelID8_Flags8_Density8_Spread8;
};

bool bitsetHas(uint bitset, uint bits) {
    return (bitset & bits) == bits;
}

uint getPixelFlags(uint packedData) {
    return (packedData >> 8u) & 0xFFu;
}

uint getPixelDensity(uint packedData) {
    return (packedData >> 16u) & 0xFFu;
}

uint getPixelSpread(uint packedData) {
    return (packedData >> 24u) & 0xFFu;
}

layout(std430, set = 1, binding = 0) readonly buffer c_s_si_SimulationData {
    float deltaTime;
    uint timer;
    float rng;
    Pixel solidPixel;
} simulationData;

layout(std430, set = 1, binding = 1) buffer na_s_PixelSSBOIn {
    Pixel readOnlyPixels[1000000];
};

layout(std430, set = 1, binding = 2) buffer s_Pixels {
    Pixel pixels[1000000];
};

const uint width = 1000;
const uint height = 1000;

const uint Solid = 1u << 0u;
const uint Gravity = 1u << 1u;
