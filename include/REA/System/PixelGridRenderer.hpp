#pragma once

#include <SplitEngine/AssetDatabase.hpp>
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Material.hpp>
#include <SplitEngine/Rendering/Model.hpp>

#include "REA/Color.hpp"
#include "REA/Component/PixelGrid.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class PixelGridRenderer final : public ECS::System<Component::PixelGrid>
	{
		public:
			explicit PixelGridRenderer(AssetHandle<Rendering::Material> material);

		protected:
			void Execute(Component::PixelGrid*, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage) override;

		private:
			struct SSBO_GridInfo
			{
				int32_t               width;
				int32_t               height;
				float                 zoom;
				alignas(16) glm::vec2 offset;
				glm::vec2             pointerPosition;
				alignas(32) Color     colorLookup[std::numeric_limits<Pixel::ID>::max() + 1];
			};

			AssetHandle<Rendering::Material>       _material;
			std::unique_ptr<Rendering::Model>      _model;
			std::ranges::iota_view<size_t, size_t> _indexes;
	};
}
