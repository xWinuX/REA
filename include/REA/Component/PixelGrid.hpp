#pragma once

#include <glm/gtc/random.hpp>

#include "REA/Pixel.hpp"


namespace REA::Component
{
	struct PixelGrid
	{
		Pixel* Pixels = nullptr;

		int32_t Width  = 1000;
		int32_t Height = 1000;
	};
}
