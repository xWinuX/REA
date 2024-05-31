#pragma once

#include <glm/vec2.hpp>

#include "REA/Pixel.hpp"


namespace REA::Component
{
	struct PixelGrid
	{
		Pixel::Data* PixelData = nullptr;

		int32_t Width  = 0;
		int32_t Height = 0;

		std::vector<Pixel> PixelLookup {};
		std::vector<Color> ColorLookup {};

		float     Zoom            = 1.0f;
		glm::vec2 Offset          = { 0, 0 };
		glm::vec2 PointerPosition = { 0, 0 };
	};
}
