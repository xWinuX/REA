#pragma once

namespace REA::Component
{
	struct Player
	{
		uint64_t  PixelGridEntityID = -1u;
		glm::vec2 Velocity{ 0.0f, 0.0f };
		bool      CanJump        = true;
		int       ColliderRadius = 8;
		bool      NoClip         = true;
		float     Acceleration   = 2000.0f;
		float     MaxSpeed       = 200.0f;
		float     Gravity        = -700.0f;
		float     JumpStrength   = 400.0f;
	};
}
