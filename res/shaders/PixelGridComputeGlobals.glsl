struct PixelData {
    uint Flags;
    uint Density;
    uint Spread;
    float TemperatureResistance;
    uint BaseTemperature;
    uint LowerTemperatureLimit;
    uint LowerTemperatureLimitPixelID;
    uint HighTemperatureLimit;
    uint HighTemperatureLimitPixelID;
};

struct Pixel {
    uint PixelID16_Temperature8_Pressure8;
};

bool bitsetHas(uint bitset, uint bits) {
    return (bitset & bits) == bits;
}

uint getPixelID(uint packedData) {
    return packedData & 0xFFFFu;
}

uint getTemperature(uint packedData) {
    return (packedData >> 16u) & 0xFFu;
}

layout(std430, set = 1, binding = 0) readonly buffer c_s_si_SimulationData {
    float deltaTime;
    uint timer;
    float rng;
    uint width;
    uint height;
    PixelData pixelLookup[1024];
} simulationData;

layout(std430, set = 1, binding = 1) buffer na_s_PixelSSBOIn {
    Pixel readOnlyPixels[1000000];
};

layout(std430, set = 1, binding = 2) buffer s_Pixels {
    Pixel pixels[1000000];
};

const uint Solid = 1u << 0u;
const uint Gravity = 1u << 1u;
