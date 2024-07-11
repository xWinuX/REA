#pragma once
#include <cstdint>
#include <glm/vec2.hpp>

namespace REA::Component
{
	struct PixelGridRigidBody
	{
		uint32_t   ShaderRigidBodyID = -1u;
		uint32_t   AllocationID      = -1u;
		uint32_t   DataIndex         = -1u;
		glm::uvec2 Size              = { 0, 0 };
	};
}
