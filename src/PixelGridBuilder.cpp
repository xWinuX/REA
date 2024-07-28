#include "REA/PixelGridBuilder.hpp"

#include <algorithm>

namespace REA
{
	PixelGridBuilder& PixelGridBuilder::WithSize(glm::ivec2 worldChunkSize, glm::ivec2 simulationChunkSize)
	{
		_worldChunkSize      = worldChunkSize;
		_simulationChunkSize = simulationChunkSize;
		return *this;
	}

	PixelGridBuilder& PixelGridBuilder::WithPixelData(std::vector<PixelCreateInfo>&& pixelData)
	{
		_pixelCreateInfos = std::move(pixelData);
		return *this;
	}

	Component::PixelGrid PixelGridBuilder::Build()
	{
		Component::PixelGrid pixelGrid{};

		std::ranges::sort(_pixelCreateInfos, [](const PixelCreateInfo& data1, const PixelCreateInfo& data2) { return data1.ID < data2.ID; });

		pixelGrid.PixelLookup.reserve(_pixelCreateInfos.size());
		pixelGrid.PixelColorLookup.reserve(_pixelCreateInfos.size());
		pixelGrid.PixelDataLookup.reserve(_pixelCreateInfos.size());

		for (const PixelCreateInfo& pixelCreateInfo: _pixelCreateInfos)
		{
			pixelGrid.PixelLookup.push_back({
				                                pixelCreateInfo.Name,
				                                {
					                                pixelCreateInfo.ID,
					                                static_cast<uint8_t>(pixelCreateInfo.Data.BaseCharge),
					                                BitSet<uint8_t>(pixelCreateInfo.Data.Flags.GetMask()),
					                                pixelCreateInfo.Data.BaseTemperature,
				                                }
			                                });
			pixelGrid.PixelColorLookup.push_back(pixelCreateInfo.Color);
			pixelGrid.PixelDataLookup.push_back(pixelCreateInfo.Data);
		}

		pixelGrid.WorldChunksX = _worldChunkSize.x;
		pixelGrid.WorldChunksY = _worldChunkSize.y;
		pixelGrid.WorldWidth   = _worldChunkSize.x * static_cast<int32_t>(Constants::CHUNK_SIZE);
		pixelGrid.WorldHeight  = _worldChunkSize.y * static_cast<int32_t>(Constants::CHUNK_SIZE);

		pixelGrid.SimulationChunksX = _simulationChunkSize.x;
		pixelGrid.SimulationChunksY = _simulationChunkSize.y;
		pixelGrid.SimulationWidth   = _simulationChunkSize.x * static_cast<int32_t>(Constants::CHUNK_SIZE);
		pixelGrid.SimulationHeight  = _simulationChunkSize.y * static_cast<int32_t>(Constants::CHUNK_SIZE);

		// Setup world chunks
		for (int i = 0; i < pixelGrid.WorldChunksX * pixelGrid.WorldChunksY; ++i)
		{
			pixelGrid.World.push_back(std::vector<Pixel::State>(Constants::NUM_ELEMENTS_IN_CHUNK, {}));
			pixelGrid.ChunkMapping.push_back(i);
		}

		_pixelCreateInfos.clear();

		return pixelGrid;
	}
}
