#pragma once
#include <glm/vec2.hpp>
#include <SplitEngine/ECS/System.hpp>

#include "ReaSystem.hpp"
#include "REA/Component/PixelGrid.hpp"
#include "REA/Component/PixelGridRenderer.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class PixelGridDrawing final : public ReaSystem<Component::PixelGrid, Component::PixelGridRenderer>
	{
		public:
			PixelGridDrawing(int radius, Pixel::ID clearPixelID);
			void ClearGrid(const Component::PixelGrid& pixelGrid);

		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;
			void Execute(Component::PixelGrid*         pixelGrids,
			             Component::PixelGridRenderer* pixelGridRenderers,
			             std::vector<uint64_t>&        entities,
			             ECS::ContextProvider&         contextProvider,
			             uint8_t                       stage) override;

		private:
			int        _radius       = 1;
			Pixel::ID  _clearPixelID = 0;
			Pixel::ID  _drawPixelID  = 0;
			glm::ivec2 _mouseWheel   = { 0, 0 };
			bool       _firstRender  = true;
	};
}
