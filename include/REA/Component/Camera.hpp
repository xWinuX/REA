#pragma once

#include <glm/vec2.hpp>

namespace REA::Component
{
	struct Camera
	{
		float     Layer          = 0.0f;
		uint64_t  TargetEntity   = -1ull;
		glm::vec2 TargetPosition = { 0.0f, 0.0f };
	};
}
