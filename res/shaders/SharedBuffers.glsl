#define SSBO_RigidBodyPixelData(setNumber, bindingNumber) \
layout (std430, set = setNumber, binding = bindingNumber) buffer s_si_dl_RigidBodyPixelData { \
    Pixel rigidBodyData[MAX_ELEMENTS]; \
};

#define SSBO_ViewportPixels(setNumber, bindingNumber) \
layout (std430, set = setNumber, binding = bindingNumber) buffer s_dl_ViewportPixels { \
    Pixel viewportPixels[MAX_ELEMENTS]; \
};


#define SSBO_PixelParticles(setNumber, bindingNumber) \
layout (std430, set = setNumber, binding = bindingNumber) buffer s_si_dl_PixelParticles { \
    int numPixelParticles; \
    int numAvailableIndices; \
    bool enabled[MAX_PIXEL_PARTICLES]; \
    uint availableIndices[MAX_PIXEL_PARTICLES]; \
    PixelParticle pixelParticles[MAX_PIXEL_PARTICLES]; \
};
