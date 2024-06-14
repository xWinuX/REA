#include "REA/System/GameOfLifeSimulation.hpp"

#include <execution>

namespace REA::System
{
	void GameOfLifeSimulation::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::PixelGrid& pixelGrid = pixelGrids[i];

			for (int pixelIndex = 0; pixelIndex < pixelGrid.ReadOnlyPixels.size(); ++pixelIndex)
			{
				const int y = pixelIndex / pixelGrid.Width;
				const int x = pixelIndex % pixelGrid.Width;

				const int bottomIndex      = pixelIndex - pixelGrid.Width;
				const int bottomRightIndex = pixelIndex - pixelGrid.Width + 1;
				const int bottomLeftIndex  = pixelIndex - pixelGrid.Width - 1;

				bool atTheBottom    = y == 0;
				bool atTheRightEdge = x == pixelGrid.Width - 1;
				bool atTheLeftEdge  = x == 0;

				if (pixelGrid.ReadOnlyPixels[pixelIndex] == 1)
				{
					if (!atTheBottom && pixelGrid.ReadOnlyPixels[bottomIndex] == 0)
					{
						pixelGrid.ReadOnlyPixels[bottomIndex] = 1;
						pixelGrid.ReadOnlyPixels[pixelIndex]  = 0;
						continue;
					}

					if (!atTheRightEdge && pixelGrid.ReadOnlyPixels[bottomRightIndex] == 0)
					{
						pixelGrid.ReadOnlyPixels[bottomRightIndex] = 1;
						pixelGrid.ReadOnlyPixels[pixelIndex]       = 0;
						continue;
					}

					if (!atTheLeftEdge && pixelGrid.ReadOnlyPixels[bottomLeftIndex] == 0)
					{
						pixelGrid.ReadOnlyPixels[bottomLeftIndex] = 1;
						pixelGrid.ReadOnlyPixels[pixelIndex]      = 0;
					}
				}


			}
		}
	}
}


/*
// Get 2D position in the grid
const int y = pixelIndex / pixelGrid.Width;
const int x = pixelIndex % pixelGrid.Width;

// Count neighbours
int aliveNeighbours = 0;
for (int yOffset = -1; yOffset <= 1; ++yOffset)
{
	for (int xOffset = -1; xOffset <= 1; ++xOffset)
	{
		// Skip self
		if (xOffset == 0 && yOffset == 0) { continue; }

		const int checkX = x + xOffset;
		const int checkY = y + yOffset;

		// Only check if not out of grid bounds
		if (checkX >= 0 && checkX < pixelGrid.Width && checkY >= 0 && checkY < pixelGrid.Height)
		{
			aliveNeighbours += pixelGrid.ReadOnlyPixels[checkY * pixelGrid.Width + checkX];
		}
	}
}

// Write current Pixel to write only grid incase none of the conditions below apply
pixelGrid.WriteOnlyPixels[pixelIndex] = pixelGrid.ReadOnlyPixels[pixelIndex];

if (pixelGrid.ReadOnlyPixels[pixelIndex] == 1)
{
	// Die
	if (aliveNeighbours < 2 || aliveNeighbours > 3) { pixelGrid.WriteOnlyPixels[pixelIndex] = 0; }
}
else
{
	// Live
	if (aliveNeighbours == 3) { pixelGrid.WriteOnlyPixels[pixelIndex] = 1; }
}*/
