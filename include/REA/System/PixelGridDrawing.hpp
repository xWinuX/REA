#pragma once
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/PixelGrid.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class PixelGridDrawing final : public ECS::System<Component::PixelGrid>
	{
		public:
			PixelGridDrawing() = default;

			void Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::Context& context) override;
	};
}
