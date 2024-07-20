#include "REA/PixelGridBuilder.hpp"

#include <algorithm>

namespace REA
{
	PixelGridBuilder& PixelGridBuilder::WithSize(glm::ivec2 worldSize, glm::ivec2 simulationSize)
	{
		_worldSize      = worldSize;
		_simulationSize = simulationSize;
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

		pixelGrid.Width            = _worldSize.x;
		pixelGrid.Height           = _worldSize.y;
		pixelGrid.SimulationWidth  = _simulationSize.x;
		pixelGrid.SimulationHeight = _simulationSize.y;

		_pixelCreateInfos.clear();

		return pixelGrid;
	}
}
