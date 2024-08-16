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

	PixelGridBuilder& PixelGridBuilder::WithPixelData(std::vector<PixelCreateInfo> pixelData)
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
				                                },
				                                pixelCreateInfo.Category,
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
		pixelGrid.World.reserve(pixelGrid.WorldChunksX * pixelGrid.WorldChunksY);
		for (int i = 0; i < pixelGrid.WorldChunksX * pixelGrid.WorldChunksY; ++i) { pixelGrid.World.push_back(std::vector<Pixel::State>(Constants::NUM_ELEMENTS_IN_CHUNK, {})); }

		// Setup chunk mapping and regenerate
		pixelGrid.ChunkMapping.reserve(pixelGrid.SimulationChunksX * pixelGrid.SimulationChunksY);
		for (int i = 0; i < pixelGrid.SimulationChunksX * pixelGrid.SimulationChunksY; ++i)
		{
			pixelGrid.ChunkMapping.push_back(i);
			pixelGrid.ChunkRegenerate.push_back(true);
		}


		pixelGrid.AvailableRigidBodyIDs = AvailableStack<uint32_t>();
		pixelGrid.RigidBodyEntities     = std::vector<Component::PixelGrid::RigidbodyEntry>();
		pixelGrid.NewRigidBodies        = std::vector<Component::PixelGrid::NewRigidBody>();
		pixelGrid.DeleteRigidbody       = std::vector<uint32_t>();


		_pixelCreateInfos.clear();

		return pixelGrid;
	}
}
