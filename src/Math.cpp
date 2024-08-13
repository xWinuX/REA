#include "REA/Math.hpp"

#include <glm/trigonometric.hpp>


namespace REA
{
	float Math::FastFmod(float a, float b) { return ((a) - ((int)((a) / (b))) * (b)); }

	int Math::Mod(int a, int b) { return (a % b + b) % b; }

	glm::vec2 Math::VecFromAngle(float angleDegrees)
	{
		const float angleRadians = glm::radians(angleDegrees);

		float x = glm::cos(angleRadians);
		float y = glm::sin(angleRadians);

		return { x, y };
	}
} // REA
