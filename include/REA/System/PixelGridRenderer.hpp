#pragma once

#include <SplitEngine/AssetDatabase.hpp>
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Material.hpp>
#include <SplitEngine/Rendering/Model.hpp>

#include "REA/Component/PixelGrid.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class PixelGridRenderer final : public ECS::System<Component::PixelGrid>
	{
		public:
			explicit PixelGridRenderer(AssetHandle<Rendering::Material> material, AssetHandle<Rendering::Texture2D> texture);

			void Execute(Component::PixelGrid*, std::vector<uint64_t>& entities, ECS::Context& context) override;
		private:
			struct SSBO_GridInfo
			{
				int32_t width;
				int32_t height;

				alignas(16) glm::vec4 colorLookup[16];
			};

			struct SSBO_GridData
			{
				std::array<uint32_t, 1'000'000> tileIDs;
			};

			AssetHandle<Rendering::Material>       _material;
			std::unique_ptr<Rendering::Model>      _model;
			std::ranges::iota_view<size_t, size_t> _indexes;
	};
}
