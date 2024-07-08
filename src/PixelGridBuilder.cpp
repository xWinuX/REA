#include "../include/REA/PixelGridBuilder.hpp"

#include <algorithm>
#include <map>
#include <box2d/b2_types.h>

namespace REA
{
	PixelGridBuilder& PixelGridBuilder::WithSize(glm::ivec2 size)
	{
		_size = size;
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
					                                BitSet<uint8>(Pixel::Flags::Static),
					                                pixelCreateInfo.Data.BaseTemperature
				                                }
			                                });
			pixelGrid.PixelColorLookup.push_back(pixelCreateInfo.Color);
			pixelGrid.PixelDataLookup.push_back(pixelCreateInfo.Data);
		}

		pixelGrid.Width  = _size.x;
		pixelGrid.Height = _size.y;

		_pixelCreateInfos.clear();

		return pixelGrid;
	}
}
