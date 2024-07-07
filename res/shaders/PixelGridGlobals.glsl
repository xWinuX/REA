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
    float TemperatureConversion;
    uint BaseCharge;
    float ChargeAbsorbtionChance;
};

struct Pixel {
    uint PixelID16_Temperature8_Pressure8;
    float Temperature;
};

bool bitsetHas(uint bitset, uint bits) {
    return (bitset & bits) == bits;
}

bool bitsetIs(uint bitset, uint bits) {
    return bitset == bits;
}


uint getPixelID(uint packedData) {
    return packedData & 0xFFFFu;
}

uint getCharge(uint packedData) {
    return (packedData >> 16u) & 0xFFu;
}

uint getDirection(uint packedData) {
    return (packedData >> 28u) & 0xFu;
}

const uint NumPixels = 1000000;
const uint NumSolid = NumPixels/32;
const uint NumMarchingSquares = uint(ceil(float(NumPixels) / 8.0f));

const int MaxCharge = 255;

const uint Solid = 1u << 7u;
const uint Gravity = 1u << 8u;
const uint Electricity = 1u << 9u;
const uint ElectricityEmitter = 1u << 10u;
const uint ElectricityReceiver = 1u << 11u;

const uint Right = 1u << 0u;
const uint Up = 1u << 1u;
const uint Left = 1u << 2u;
const uint Down = 1u << 3u;

const uint RenderMode_Normal = 0;
const uint RenderMode_Temperature = 1;