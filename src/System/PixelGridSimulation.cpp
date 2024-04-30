#include "REA/System/PixelGridSimulation.hpp"

#include <execution>
#include <SplitEngine/Input.hpp>

namespace REA::System
{
	PixelGridSimulation::PixelGridSimulation()
	{
		/*
		for (uint32_t& tileID: _gridData->tileIDs)
		{
			tileID = glm::linearRand(0u, 1u);
		}*/
	}

	void PixelGridSimulation::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::Context& context)
	{
		for (int entityIndex = 0; entityIndex < entities.size(); ++entityIndex)
		{
			Component::PixelGrid& pixelGrid = pixelGrids[entityIndex];


			//memcpy(_tempGrid.Pixels.data(), pixelGrid.Pixels.data(), pixelGrid.Pixels.size() * sizeof(Pixel));


			std::vector<Pixel>& readOnlyPixels  = pixelGrid.Pixels;
			std::vector<Pixel>& writeOnlyPixels = _tempGrid.Pixels;

			const int width  = pixelGrid.Width;
			const int height = pixelGrid.Height;


			size_t divider   = 64;
			size_t chunkSize = readOnlyPixels.size() / divider;

			_indexes = std::ranges::iota_view((size_t)0, divider);

			//std::ranges::fill(_swapList, -1);

			Pixel solidPixel = { 99, BitSet<uint8_t>(Solid), std::numeric_limits<int8_t>::max(), 0 };


			// Falling
			std::for_each(std::execution::par_unseq,
			              _indexes.begin(),
			              _indexes.end(),
			              [this, readOnlyPixels, chunkSize, solidPixel, width, height](size_t chunkIndex)
			              {
				              size_t start = chunkIndex * chunkSize;
				              size_t limit = std::min((chunkSize * (chunkIndex + 1)), readOnlyPixels.size());
				              for (size_t j = start; j < limit; ++j)
				              {
					              const int y = j / width;
					              const int x = j % height;

					              Pixel currentPixel = readOnlyPixels[j];

					              if (currentPixel.Flags.Has(Gravity))
					              {
						              bool atTheBottom = y == 0;
						              bool atTheTop    = y == height - 1;

						              int topIndex    = (y + 1) * width + x;
						              int bottomIndex = (y - 1) * width + x;

						              // Middle
						              Pixel topPixel = solidPixel;
						              if (!atTheTop) { topPixel = readOnlyPixels[topIndex]; }

						              Pixel bottomPixel = solidPixel;
						              if (!atTheBottom) { bottomPixel = readOnlyPixels[bottomIndex]; }

						              Pixel rightPixel       = solidPixel;
						              Pixel topRightPixel    = solidPixel;
						              Pixel bottomRightPixel = solidPixel;


						              Pixel leftPixel       = solidPixel;
						              Pixel topLeftPixel    = solidPixel;
						              Pixel bottomLeftPixel = solidPixel;

						              // Right
						              if (x + 1 < width)
						              {
							              rightPixel = readOnlyPixels[j + 1];

							              if (!atTheTop) { topRightPixel = readOnlyPixels[topIndex + 1]; }

							              if (!atTheBottom) { bottomRightPixel = readOnlyPixels[bottomIndex + 1]; }
						              }

						              // Left
						              if (x - 1 >= 0)
						              {
							              leftPixel = readOnlyPixels[j - 1];

							              if (!atTheTop) { topLeftPixel = readOnlyPixels[topIndex - 1]; }

							              if (!atTheBottom) { bottomLeftPixel = readOnlyPixels[bottomIndex - 1]; }
						              }


						              if (_timer % 2 == 0)
						              {
							              if (!topPixel.Flags.Has(Solid) && currentPixel.Density < topPixel.Density)
							              {
								              _tempGrid.Pixels[j] = topPixel;
								              continue;
							              }

							              if (!bottomPixel.Flags.Has(Solid) && currentPixel.Density > bottomPixel.Density)
							              {
								              _tempGrid.Pixels[j] = bottomPixel;
								              continue;
							              }

							              if (!topLeftPixel.Flags.Has(Solid) && !leftPixel.Flags.Has(Solid) && leftPixel.Density >= topLeftPixel.Density && currentPixel.Density <
							                  topLeftPixel.Density)
							              {
								              _tempGrid.Pixels[j] = topLeftPixel;
								              continue;
							              }

							              if (!bottomRightPixel.Flags.Has(Solid) && currentPixel.Density > bottomRightPixel.Density)
							              {
								              _tempGrid.Pixels[j] = bottomRightPixel;
								              continue;
							              }
						              }
						              else
						              {
							              if (!topPixel.Flags.Has(Solid) && currentPixel.Density < topPixel.Density)
							              {
								              _tempGrid.Pixels[j] = topPixel;
								              continue;
							              }

							              if (!topRightPixel.Flags.Has(Solid) && !rightPixel.Flags.Has(Solid) && rightPixel.Density >= topRightPixel.Density && currentPixel.Density
							                  < topRightPixel.Density)
							              {
								              _tempGrid.Pixels[j] = topRightPixel;
								              continue;
							              }

							              if (!bottomPixel.Flags.Has(Solid) && currentPixel.Density > bottomPixel.Density)
							              {
								              _tempGrid.Pixels[j] = bottomPixel;
								              continue;
							              }

							              if (!bottomLeftPixel.Flags.Has(Solid) && currentPixel.Density > bottomLeftPixel.Density)
							              {
								              _tempGrid.Pixels[j] = bottomLeftPixel;
								              continue;
							              }
						              }

						              _tempGrid.Pixels[j] = currentPixel;
					              }
				              }
			              });


			float rng = glm::linearRand(0, 1);

			for (int flowIteration = 0; flowIteration < 1; ++flowIteration)
			{
				std::swap(_tempGrid.Pixels, pixelGrid.Pixels);

				//readOnlyPixels  = pixelGrid.Pixels;
				//writeOnlyPixels = _tempGrid.Pixels;


				// Flow
				std::for_each(std::execution::par_unseq,
				              _indexes.begin(),
				              _indexes.end(),
				              [this, readOnlyPixels, rng, chunkSize, solidPixel, width, height](size_t chunkIndex)
				              {
					              size_t start = chunkIndex * chunkSize;
					              size_t limit = std::min((chunkSize * (chunkIndex + 1)), readOnlyPixels.size());
					              for (size_t j = start; j < limit; ++j)
					              {
						              const int y = j / width;
						              const int x = j % height;

						              Pixel currentPixel = readOnlyPixels[j];

						              if (currentPixel.Flags.Has(Gravity))
						              {
							              bool atTheBottom = y == 0;
							              bool atTheTop    = y == height - 1;

							              int topIndex    = (y + 1) * width + x;
							              int bottomIndex = (y - 1) * width + x;


							              Pixel bottomPixel = solidPixel;
							              if (!atTheBottom) { bottomPixel = readOnlyPixels[bottomIndex]; }

							              Pixel rightPixel       = solidPixel;
							              Pixel bottomRightPixel = solidPixel;


							              Pixel leftPixel       = solidPixel;
							              Pixel bottomLeftPixel = solidPixel;

							              // Right
							              if (x + 1 < width)
							              {
								              rightPixel = readOnlyPixels[j + 1];

								              if (!atTheBottom) { bottomRightPixel = readOnlyPixels[bottomIndex + 1]; }
							              }

							              // Left
							              if (x - 1 >= 0)
							              {
								              leftPixel = readOnlyPixels[j - 1];

								              if (!atTheBottom) { bottomLeftPixel = readOnlyPixels[bottomIndex - 1]; }
							              }

							              if (rng > 0.5)
							              {
								              if (!leftPixel.Flags.Has(Solid) && bottomLeftPixel.Density >= leftPixel.Density && leftPixel.SpreadingFactor > 0 && currentPixel.
								                  Density < leftPixel.Density)
								              {
									              _tempGrid.Pixels[j] = leftPixel;
									              continue;
								              }

								              if (!rightPixel.Flags.Has(Solid) && bottomPixel.Density >= currentPixel.Density && currentPixel.SpreadingFactor > 0 && currentPixel.
								                  Density >= rightPixel.Density)
								              {
									              _tempGrid.Pixels[j] = rightPixel;
									              continue;
								              }
							              }
							              else
							              {
								              if (!rightPixel.Flags.Has(Solid) && bottomRightPixel.Density >= rightPixel.Density && rightPixel.SpreadingFactor > 0 && currentPixel.
								                  Density < rightPixel.Density)
								              {
									              _tempGrid.Pixels[j] = rightPixel;
									              continue;
								              }

								              if (!leftPixel.Flags.Has(Solid) && bottomPixel.Density >= currentPixel.Density && currentPixel.SpreadingFactor > 0 && currentPixel.
								                  Density >= leftPixel.Density)
								              {
									              _tempGrid.Pixels[j] = leftPixel;
									              continue;
								              }
							              }


							              _tempGrid.Pixels[j] = currentPixel;
						              }
					              }
				              });
			}

			std::swap(_tempGrid.Pixels, pixelGrid.Pixels);
		}

		_timer++;
	}
}
