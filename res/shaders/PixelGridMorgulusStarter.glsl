uint width = simulationData.width;
uint height = simulationData.height;

if (gl_GlobalInvocationID.x >= (width * height) / 4) {
    return;
}

uint cellStep = gl_GlobalInvocationID.x * 2;
uint margolusX = ((cellStep + margolusOffset.x) % width);
uint margolusY = (((cellStep / width) * 2) + 1) - margolusOffset.y;

uint topLeftIndex = (margolusY * width) + margolusX;

uint x = margolusX;
uint y = margolusY;

uint topRightX = (x + 1) % width;
uint topRightY = y;

uint bottomLeftX = x;
uint bottomLeftY = (y == 0) ? height - 1 : y - 1;

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

PixelData topLeftPixelData = simulationData.pixelLookup[topLeftPixel.PixelID16_Temperature8_Pressure8 & 0xFFFFu];
PixelData bottomLeftPixelData = simulationData.pixelLookup[bottomLeftPixel.PixelID16_Temperature8_Pressure8 & 0xFFFFu];
PixelData topRightPixelData = simulationData.pixelLookup[topRightPixel.PixelID16_Temperature8_Pressure8 & 0xFFFFu];
PixelData bottomRightPixelData = simulationData.pixelLookup[bottomRightPixel.PixelID16_Temperature8_Pressure8 & 0xFFFFu];

// Write pixels
pixels[topLeftIndex] = topLeftPixel;
pixels[bottomLeftIndex] = bottomLeftPixel;
pixels[topRightIndex] = topRightPixel;
pixels[bottomRightIndex] = bottomRightPixel;
