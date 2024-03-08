#pragma once
#include <glm/vec3.hpp>

namespace REA::Component
{
	struct Transform
	{
		glm::vec3 Position{};
		float     Rotation = 0;
	};
}
