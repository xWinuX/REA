#pragma once

#include <glm/vec2.hpp>

#include "REA/Pixel.hpp"


namespace REA::Component
{
	struct PixelGrid
	{
		Pixel::State* PixelState = nullptr;

		int32_t Width  = 0;
		int32_t Height = 0;

		std::vector<Pixel>       PixelLookup{};
		std::vector<Pixel::Data> PixelDataLookup;
		std::vector<Color>       PixelColorLookup{};
	};
}
