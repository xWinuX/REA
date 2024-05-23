#pragma once

#include <vector>

namespace REA::Component
{
	struct Camera
	{
		uint64_t  TargetEntity   = -1u;
		glm::vec2 TargetPosition = { 0.0f, 0.0f };
	};
}
