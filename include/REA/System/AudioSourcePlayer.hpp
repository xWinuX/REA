#pragma once
#include <ranges>
#include <SplitEngine/ECS/ContextProvider.hpp>
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/AudioSource.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class AudioSourcePlayer final : public ECS::System<Component::AudioSource>
	{
		public:
			AudioSourcePlayer() = default;

		protected:
			void Execute(Component::AudioSource* audioSourceComponents, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage) override;

		private:
			std::ranges::iota_view<size_t, size_t> _indexes;
	};
}
