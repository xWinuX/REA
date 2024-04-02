#include "REA/System/GameOfLifeSimulation.hpp"

#include <execution>

namespace REA::System
{
	void GameOfLifeSimulation::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::Context& context)
	{
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::PixelGrid& pixelGrid = pixelGrids[i];

			int32_t width  = pixelGrid.Width;
			int32_t height = pixelGrid.Height;

			_indexes = std::ranges::iota_view<size_t, size_t>(1, 1'000'000);
			std::for_each(std::execution::par,
			              _indexes.begin(),
			              _indexes.end(),
			              [this, width, height, pixelGrid](const size_t i)
			              {
				              const int y     = i / width;
				              const int x     = i % width;
				              const int index = y * width + x;

				              const auto& tileIDs    = pixelGrid.Pixels;
				              auto&       newTileIDs = _staticGrid.Pixels;

				              const Pixel& pixel = pixelGrid.Pixels[index];

				              int liveNeighbors = 0;

				              // Top left neighbor
				              if (x - 1 >= 0 && y - 1 >= 0 && tileIDs[(y - 1) * width + (x - 1)].PixelID == 1) { liveNeighbors++; }

				              // Top neighbor
				              if (y - 1 >= 0 && tileIDs[(y - 1) * width + x].PixelID == 1) { liveNeighbors++; }

				              // Top right neighbor
				              if (x + 1 < width && y - 1 >= 0 && tileIDs[(y - 1) * width + (x + 1)].PixelID == 1) { liveNeighbors++; }

				              // Left neighbor
				              if (x - 1 >= 0 && tileIDs[y * width + (x - 1)].PixelID == 1) { liveNeighbors++; }

				              // Right neighbor
				              if (x + 1 < width && tileIDs[y * width + (x + 1)].PixelID == 1) { liveNeighbors++; }

				              // Bottom left neighbor
				              if (x - 1 >= 0 && y + 1 < height && tileIDs[(y + 1) * width + (x - 1)].PixelID == 1) { liveNeighbors++; }

				              // Bottom neighbor
				              if (y + 1 < height && tileIDs[(y + 1) * width + x].PixelID == 1) { liveNeighbors++; }

				              // Bottom right neighbor
				              if (x + 1 < width && y + 1 < height && tileIDs[(y + 1) * width + (x + 1)].PixelID == 1) { liveNeighbors++; }

				              newTileIDs[index].PixelID = pixel.PixelID;

				              if (pixel.PixelID == 1) { if (liveNeighbors < 2 || liveNeighbors > 3) { newTileIDs[index].PixelID = 0; } }
				              else
				              {
					              if (liveNeighbors == 3)
					              {
						              newTileIDs[index].PixelID = 1; // Resurrect
					              }
				              }
			              });

			std::swap(pixelGrid, _staticGrid);

		}
	}
}
