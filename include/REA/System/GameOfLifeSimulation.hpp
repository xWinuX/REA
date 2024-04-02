#pragma once
#include <ranges>
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/PixelGrid.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class GameOfLifeSimulation final : public ECS::System<Component::PixelGrid>
	{
		public:
			void Execute(Component::PixelGrid*, std::vector<uint64_t>& entities, ECS::Context& context) override;

		private:
			std::ranges::iota_view<size_t, size_t> _indexes;
			Component::PixelGrid _staticGrid {};
	};
}
