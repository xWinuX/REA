#pragma once
#include <ranges>
#include <SplitEngine/ECS/Context.hpp>
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/AudioSource.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class AudioSourcePlayer final : public ECS::System<Component::AudioSource>
	{
		public:
			AudioSourcePlayer() = default;

			void Execute(Component::AudioSource* audioSourceComponents, std::vector<uint64_t>& entities, ECS::Context& context) override;

		private:
			std::ranges::iota_view<size_t, size_t> _indexes;
	};
}
