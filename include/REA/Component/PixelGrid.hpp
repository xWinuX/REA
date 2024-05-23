#pragma once

#include <glm/vec2.hpp>

#include "REA/Pixel.hpp"


namespace REA::Component
{
	struct PixelGrid
	{
		Pixel* Pixels = nullptr;

		int32_t   Width  = 1000;
		int32_t   Height = 1000;
		float     Zoom   = 5.0f;
		glm::vec2 Offset = { 0, 0 };
	};
}
