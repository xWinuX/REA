#include "PixelGridGlobals.glsl"

layout (std430, set = 1, binding = 0) buffer s_si_SimulationData {
    float deltaTime;
    uint timer;
    float rng;
    ivec2 chunkOffset;
    uint chunkMapping[NUM_CHUNKS];
    PixelData pixelLookup[1024];
} simulationData;

layout (std430, set = 1, binding = 1) buffer s_si_Updates  {
    bool regenerateChunks[NUM_CHUNKS];
};

layout (std430, set = 1, binding = 2) buffer s_si_RigidBodies {
    RigidBody rigidBodies[MAX_RIGIDBODIES];
};

layout (std430, set = 1, binding = 3) buffer s_ts_ReadWorldPixels {
    Pixel readPixels[NUM_CHUNKS][NUM_ELEMENTS_IN_CHUNK];
};

layout (std430, set = 1, binding = 4) buffer s_na_ts_WriteWorldPixels {
    Pixel writePixels[NUM_CHUNKS][NUM_ELEMENTS_IN_CHUNK];
};

#define SetupPixelVars(name, index) \
uint name##ChunkIndex = index / NUM_ELEMENTS_IN_CHUNK; \
uint name##PixelIndex = index % NUM_ELEMENTS_IN_CHUNK; \
uint name##ChunkMapping = simulationData.chunkMapping[name##ChunkIndex]; \
Pixel name##Pixel = readPixels[name##ChunkMapping][name##PixelIndex];

#define SetupWritePixelVars(name, index) \
uint name##ChunkIndex = index / NUM_ELEMENTS_IN_CHUNK; \
uint name##PixelIndex = index % NUM_ELEMENTS_IN_CHUNK; \
uint name##ChunkMapping = simulationData.chunkMapping[name##ChunkIndex]; \
Pixel name##Pixel = readPixels[name##ChunkMapping][name##PixelIndex];

#define GetPixelPosition(name) uvec2( \
    (name##ChunkIndex % CHUNKS_X) * CHUNK_SIZE + (name##PixelIndex % CHUNK_SIZE), \
    (name##ChunkIndex / CHUNKS_X) * CHUNK_SIZE + (name##PixelIndex / CHUNK_SIZE) \
)

#define SetupPixelVarsByPosition(name, x, y) \
uint name##ChunkIndex = (y / CHUNK_SIZE) * CHUNKS_X + (x / CHUNK_SIZE); \
uint name##PixelIndex = (y % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE); \
uint name##ChunkMapping = simulationData.chunkMapping[name##ChunkIndex]; \
Pixel name##Pixel = readPixels[name##ChunkMapping][name##PixelIndex];

#define SetupWritePixelVarsByPosition(name, x, y) \
uint name##ChunkIndex = (y / CHUNK_SIZE) * CHUNKS_X + (x / CHUNK_SIZE); \
uint name##PixelIndex = (y % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE); \
uint name##ChunkMapping = simulationData.chunkMapping[name##ChunkIndex]; \
Pixel name##Pixel = writePixels[name##ChunkMapping][name##PixelIndex];

#define BoundaryCheck() if (gl_GlobalInvocationID.x >= MAX_ELEMENTS) { return; }

#define GetWritePixel(name) writePixels[name##ChunkMapping][name##PixelIndex]
#define GetReadPixel(name) readPixels[name##ChunkMapping][name##PixelIndex]

#define GetPixel(name) name##Pixel

#define PositionToIndex(position) position.y * NUM_ELEMENTS_X + position.x
