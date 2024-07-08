layout(set = 1, binding = 4) buffer s_dl_Labels {
    int labels[NumPixels];
};

uint getGlobalIndex(uint index, uint globalWidth, uvec2 localSize, uvec2 globalOffset)
{
    uint x = globalOffset.x + (gl_GlobalInvocationID.x % localSize.x);
    uint y = globalOffset.y + (gl_GlobalInvocationID.x / localSize.x);

    return y * globalWidth + x;
}

uint getGlobalIndex(uint x, uint y, uint globalWidth, uvec2 globalOffset)
{
    x += globalOffset.x;
    y += globalOffset.y;

    return y * globalWidth + x;
}