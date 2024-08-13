#pragma once

namespace REA::Component
{
	struct Player
	{
		uint64_t  PixelGridEntityID = -1u;
		glm::vec2 Position{ 0.0f, 0.0f };
		glm::vec2 Velocity{ 0.0f, 0.0f };
		bool      CanJump        = true;
		float     ColliderRadius = 8.0f;
	};
}
