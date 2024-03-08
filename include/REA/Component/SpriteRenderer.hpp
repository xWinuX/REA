#pragma once
#include <SplitEngine/AssetDatabase.hpp>
#include <REA/SpriteTexture.hpp>

using namespace SplitEngine;

namespace REA::Component
{
	struct SpriteRenderer
	{
		AssetHandle<SpriteTexture> SpriteTexture;
		float                      AnimationSpeed    = 0;
		float                      CurrentFrame      = 0;
		uint32_t                   PreviousTextureID = 0;
	};
}
