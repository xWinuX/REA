#include "../include/REA/PixelGridBuilder.hpp"

#include <algorithm>

namespace REA
{
	PixelGridBuilder& PixelGridBuilder::WithSize(glm::ivec2 size)
	{
		_size = size;
		return *this;
	}

	PixelGridBuilder& PixelGridBuilder::WithPixelData(std::vector<Pixel>&& pixelData)
	{
		_pixels = std::move(pixelData);
		return *this;
	}

	Component::PixelGrid PixelGridBuilder::Build()
	{
		Component::PixelGrid pixelGrid{};

		std::ranges::sort(_pixels, [](const Pixel& data1, const Pixel& data2) { return data1.PixelData.PixelID < data2.PixelData.PixelID; });

		pixelGrid.ColorLookup.reserve(_pixels.size());
		for (const Pixel& pixel: _pixels) { pixelGrid.ColorLookup.push_back(pixel.Color); }

		pixelGrid.PixelLookup = std::move(_pixels);

		pixelGrid.Width  = _size.x;
		pixelGrid.Height = _size.y;

		_pixels = {};

		return pixelGrid;
	}
}
