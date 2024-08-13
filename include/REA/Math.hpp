#pragma once

#include <glm/vec2.hpp>

namespace REA
{
	class Math
	{
		public:
			static float     FastFmod(float a, float b);
			static int       Mod(int a, int b);
			static glm::vec2 VecFromAngle(float angleDegrees);
	};
}
