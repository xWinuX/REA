#pragma once

#include <glm/gtc/random.hpp>

#include "REA/Pixel.hpp"


namespace REA::Component
{
	struct PixelGrid
	{
		std::vector<Pixel> Pixels = std::vector<Pixel>(1'000'000, Pixel());

		int32_t Width  = 100;
		int32_t Height = 100;
	};
}
