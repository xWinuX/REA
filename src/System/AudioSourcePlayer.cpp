// This needs to be here because soloud overrides numericLimits::max (fucking hell)
#include <SplitEngine/Contexts.hpp>

#include "REA/System/AudioSourcePlayer.hpp"
#include <SplitEngine/Audio/Manager.hpp>
#include <SplitEngine/Audio/SoundEffect.hpp>

#include <execution>

namespace REA::System
{
	void AudioSourcePlayer::Execute(Component::AudioSource* audioSourceComponents, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		_indexes = std::ranges::iota_view(static_cast<size_t>(0), entities.size());

		Audio::Manager* audioManager = contextProvider.GetContext<AudioContext>()->AudioManager;
		std::for_each(std::execution::par,
		              _indexes.begin(),
		              _indexes.end(),
		              [audioSourceComponents, audioManager](const size_t i)
		              {
			              AssetHandle<Audio::SoundEffect>& soundEffect = audioSourceComponents[i].SoundEffect;
			              if (audioSourceComponents[i].Play)
			              {
				              audioManager->PlaySound(soundEffect, 1.0f);
				              audioSourceComponents[i].Play = false;
			              }
		              });
	}
}
