#pragma once

#include <glm/vec2.hpp>
#include <SplitEngine/DataStructures.hpp>


#include "REA/Pixel.hpp"


namespace REA::Component
{
	struct PixelGrid
	{
		Pixel::State* PixelState = nullptr;

		int32_t Width  = 500;
		int32_t Height = 500;

		std::vector<Pixel>       PixelLookup{};
		std::vector<Pixel::Data> PixelDataLookup;
		std::vector<Color>       PixelColorLookup{};

		std::vector<uint32_t> ReadOnlyPixels  = std::vector<uint32_t>(Width * Height, 0);
		std::vector<uint32_t> WriteOnlyPixels = std::vector<uint32_t>(Width * Height, 0);
	};
}
