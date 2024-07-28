
const uint CHUNKS_X = 8;
const uint CHUNKS_Y = 8;
const uint CHUNK_SIZE = 128;

const uint NUM_CHUNKS = CHUNKS_X * CHUNKS_Y;
const uint NUM_ELEMENTS_IN_CHUNK = CHUNK_SIZE * CHUNK_SIZE;
const uint MAX_ELEMENTS = NUM_ELEMENTS_IN_CHUNK * NUM_CHUNKS;

const uint NUM_ELEMENTS_X = CHUNKS_X * CHUNK_SIZE;
const uint NUM_ELEMENTS_Y = CHUNKS_Y * CHUNK_SIZE;

const vec2 GRID_SIZE_F = vec2(NUM_ELEMENTS_X, NUM_ELEMENTS_Y);

const uint NumSimulatedPixels = MAX_ELEMENTS; // 1024 * 1024
const uint NumPixels = 8388608 + NumSimulatedPixels; // 4096 x 2048 + Simulated Pixels for ping ponging buffer swapping

const uint NumRigidbodies = 1024;
const uint NumMarchingSquareSegments = 100000;

const int MaxCharge = 255;

const uint Solid = 1u << 0u;
const uint Connected = 1u << 1u;
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

struct PixelData {
    uint Flags;
    uint FlagsCarryover;
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
    uint PixelID16_Charge8_Flags8;
    float Temperature;
    uint RigidBodyID12_RigidBodyIndex20;
};

struct RigidBody {
    uint ID;
    uint DataIndex;
    bool NeedsRecalculation;
    float Rotation;
    vec2 Position;
    uvec2 Size;
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

uint getFlags(uint packedData) {
    return (packedData >> 24u) & 0xFFu;
}

uint getRigidBodyID(uint packedData) {
    return packedData & 0xFFFu;
}

uint getRigidBodyIndex(uint packedData) {
    return (packedData >> 12u) & 0xFFFFFu;
}
uvec2 getChunkPosition(uint index)
{
    uint chunkIndex = index / NUM_ELEMENTS_IN_CHUNK;

    return uvec2(chunkIndex % CHUNKS_X, chunkIndex / CHUNKS_X);
}

uvec2 getChunkPosition(uint index, uvec2 chunkOffset)
{
    uint chunkIndex = index / NUM_ELEMENTS_IN_CHUNK;

    return chunkOffset + uvec2(chunkIndex % CHUNKS_X, chunkIndex / CHUNKS_X);
}

uint getGlobalIndex(uint index, uint globalWidth, uvec2 localSize, uvec2 globalOffset)
{
    uint x = globalOffset.x + (index % localSize.x);
    uint y = globalOffset.y + (index / localSize.x);

    return y * globalWidth + x;
}

uint getGlobalIndex(uint x, uint y, uint globalWidth, uvec2 globalOffset)
{
    x += globalOffset.x;
    y += globalOffset.y;

    return y * globalWidth + x;
}
