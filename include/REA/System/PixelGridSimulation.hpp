#pragma once
#include <ranges>
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/PixelGrid.hpp"

namespace REA::System
{
	class PixelGridSimulation final : public ECS::System<Component::PixelGrid>
	{
		public:
			PixelGridSimulation();

			void Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, SplitEngine::ECS::Context& context) override;

		private:
			uint32_t _timer;

			std::ranges::iota_view<size_t, size_t> _indexes;

			Component::PixelGrid _tempGrid{};

			std::vector<int32_t> _swapList = std::vector<int32_t>(1'000'000, 0);
	};
}
