#pragma once

#include <REA/Sprite.hpp>
#include <SplitEngine/AssetDatabase.hpp>

namespace REA::Component
{
	struct Player
	{
		AssetHandle<Sprite> IdleSpriteR{};
		AssetHandle<Sprite> WalkSpriteR{};
		AssetHandle<Sprite> IdleSpriteL{};
		AssetHandle<Sprite> WalkSpriteL{};

		uint64_t  PixelGridEntityID = -1u;
		glm::vec2 Velocity{ 0.0f, 0.0f };
		int       FaceDirection  = 1;
		bool      CanJump        = true;
		int       ColliderRadius = 8;
		bool      NoClip         = false;
		float     Acceleration   = 2000.0f;
		float     MaxSpeed       = 200.0f;
		float     Gravity        = -700.0f;
		float     JumpStrength   = 400.0f;
	};
}
