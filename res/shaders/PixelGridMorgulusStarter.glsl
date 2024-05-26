uint cellStep = gl_GlobalInvocationID.x * 2;
uint margolusX = (cellStep % width) + margolusOffset.x;
uint margolusY = ((cellStep / width) * 2) + margolusOffset.y;

uint topLeftIndex = (margolusY * width) + margolusX;

// Early exit if topLeftIndex is out of bounds
if (topLeftIndex >= width * height) {
    return;
}

Pixel solidPixel = simulationData.solidPixel;

uint y = margolusY;
uint x = margolusX;

// Calculate wrapped indices
uint topRightIndex = topLeftIndex + 1;

uint wrappedY = (y == 0) ? height - 1 : y - 1;
uint bottomLeftIndex = wrappedY * width + x;

uint bottomRightX = (x + 1) % width;
uint bottomRightIndex = bottomLeftIndex + bottomRightX - x;

// Read pixels
Pixel topLeftPixel = readOnlyPixels[topLeftIndex];
Pixel bottomLeftPixel = readOnlyPixels[bottomLeftIndex];
Pixel topRightPixel = readOnlyPixels[topRightIndex];
Pixel bottomRightPixel = readOnlyPixels[bottomRightIndex];

// Write pixels
pixels[topLeftIndex] = topLeftPixel;
pixels[bottomLeftIndex] = bottomLeftPixel;
pixels[topRightIndex] = topRightPixel;
pixels[bottomRightIndex] = bottomRightPixel;
