struct PixelData {
    uint Flags;
    uint Density;
    uint Spread;
    float TemperatureResistance;
    float BaseTemperature;
    float LowerTemperatureLimit;
    uint LowerTemperatureLimitPixelID;
    float HighTemperatureLimit;
    uint HighTemperatureLimitPixelID;
};

struct Pixel {
    uint PixelID16_Temperature8_Pressure8;
    float Temperature;
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


const uint Solid = 1u << 0u;
const uint Gravity = 1u << 1u;


const uint RenderMode_Normal = 0;
const uint RenderMode_Temperature = 1;