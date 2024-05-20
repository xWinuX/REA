#pragma once
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/PixelGrid.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class PixelGridDrawing final : public ECS::System<Component::PixelGrid>
	{
		public:
			PixelGridDrawing(int radius = 1);

		protected:
			void Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage) override;

		private:
			int _radius = 1;
	};
}
