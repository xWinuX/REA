// Early exit if topLeftIndex is out of bounds
if (gl_GlobalInvocationID.x >= (width * height) / 4) {
    return;
}

uint cellStep = gl_GlobalInvocationID.x * 2;
uint margolusX = ((cellStep + margolusOffset.x) % width);
uint margolusY = (((cellStep / width) * 2) + 1) - margolusOffset.y;

uint topLeftIndex = (margolusY * width) + margolusX;


Pixel solidPixel = simulationData.solidPixel;

uint x = margolusX;
uint y = margolusY;

uint topRightX = (x + 1) % width;
uint topRightY = y;

uint bottomLeftX = x;
uint bottomLeftY = int(y)-1 == -1 ? height - 1 : y - 1;

uint bottomRightX = topRightX;
uint bottomRightY = bottomLeftY;

// Calculate wrapped indices
uint topRightIndex = (topRightY * width) + topRightX;
uint bottomLeftIndex = (bottomLeftY * width) + bottomLeftX;
uint bottomRightIndex = (bottomRightY * width) + bottomRightX;

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
