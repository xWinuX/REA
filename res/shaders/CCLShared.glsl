layout(set = 1, binding = 5) buffer s_si_Labels {
    int labels[NumPixels];
};

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