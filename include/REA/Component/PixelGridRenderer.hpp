#pragma once
#include <glm/vec2.hpp>

namespace REA::Component
{
	enum RenderMode : uint32_t
	{
		Normal,
		Temperature,
		MAX_VALUE
	};

	struct PixelGridRenderer
	{
		float      Zoom            = 1.0f;
		glm::vec2  Offset          = { 0, 0 };
		glm::vec2  PointerPosition = { 0, 0 };
		RenderMode RenderMode      = RenderMode::Normal;
	};
}
