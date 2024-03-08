#pragma once
#include <SplitEngine/AssetDatabase.hpp>
#include <SplitEngine/Audio/SoundEffect.hpp>

namespace REA::Component
{
	struct AudioSource
	{
		SplitEngine::AssetHandle<SplitEngine::Audio::SoundEffect> SoundEffect;

		bool Play = false;
	};
}
