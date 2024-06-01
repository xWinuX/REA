#pragma once
#include <glm/vec2.hpp>
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/PixelGrid.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class PixelGridDrawing final : public ECS::System<Component::PixelGrid>
	{
		public:
			PixelGridDrawing(int radius);

		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;
			void Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage) override;

		private:
			int        _radius      = 1;
			Pixel::ID  _drawPixelID = 0;
			glm::ivec2 _mouseWheel  = { 0, 0 };
	};
}
