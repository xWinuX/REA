#pragma once
#include <SplitEngine/AssetDatabase.hpp>
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Material.hpp>
#include <SplitEngine/Rendering/Model.hpp>

using namespace SplitEngine;

namespace REA::System
{
	class PixelGridRenderer final : public ECS::SystemBase
	{
		public:
			explicit PixelGridRenderer(AssetHandle<Rendering::Material> material);
			void RunExecute(ECS::Context& context) override;

		private:
			struct TileData
			{
				glm::vec4 colorLookup[16];
			};

			struct GridBuffer
			{
				uint32_t tileIDs[10000];
			};

			AssetHandle<Rendering::Material>  _material;
			std::unique_ptr<Rendering::Model> _model;
	};
}
