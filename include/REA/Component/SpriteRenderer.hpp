#pragma once
#include <SplitEngine/AssetDatabase.hpp>
#include <REA/Sprite.hpp>

using namespace SplitEngine;

namespace REA::Component
{
	struct SpriteRenderer
	{
		AssetHandle<Sprite> SpriteTexture;
		float               AnimationSpeed    = 0;
		float               CurrentFrame      = 0;
		uint32_t            PreviousTextureID = 0;
	};
}
