#pragma once

#include <SplitEngine/AssetDatabase.hpp>
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Material.hpp>
#include <SplitEngine/Rendering/Model.hpp>
#include <SplitEngine/DataStructures.hpp>

#include "REA/Component/PixelGrid.hpp"
#include "REA/Component/PixelGridRenderer.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class PixelGridRenderer final : public ECS::System<Component::PixelGrid, Component::PixelGridRenderer>
	{
		public:
			explicit PixelGridRenderer(AssetHandle<Rendering::Material> material);

		protected:
			void Execute(Component::PixelGrid*         pixelGrids,
			             Component::PixelGridRenderer* pixelGridRenderers,
			             std::vector<uint64_t>&        entities,
			             ECS::ContextProvider&         contextProvider,
			             uint8_t                       stage) override;

		private:
			struct SSBO_GridInfo
			{
				int32_t               width;
				int32_t               height;
				float                 zoom;
				Component::RenderMode renderMode;
				glm::vec2             offset;
				glm::vec2             pointerPosition;
				Color                 colorLookup[256];
			};

			AssetHandle<Rendering::Material>       _material;
			std::unique_ptr<Rendering::Model>      _model;
			std::ranges::iota_view<size_t, size_t> _indexes;

			uint64_t _previousEntity  = -1;
			uint32_t _sameGridCounter = 0;
	};
}
