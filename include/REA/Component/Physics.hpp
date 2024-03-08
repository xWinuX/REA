#pragma once
#include <glm/vec3.hpp>

namespace REA::Component
{
	struct Physics
	{
		bool      HasGravity = false;
		glm::vec3 Velocity{};
	};
}
