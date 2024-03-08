#pragma once
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Material.hpp>
#include <SplitEngine/Rendering/Model.hpp>
#include <REA/Component/Transform.hpp>
#include <REA/Component/SpriteRenderer.hpp>


using namespace SplitEngine;

namespace REA::System
{
	class SpriteRenderer : public ECS::System<Component::Transform, Component::SpriteRenderer>
	{
		public:
			SpriteRenderer(AssetHandle<Rendering::Material> material, Tools::ImagePacker::PackingData& packingData);

			/**
			 * This function can be override if you need more control over how archetypes are handled
			 *
			 * In this example we want to apply animation and positions to all entities with sprite and transform components.
			 * Since this is a render system we also want to give the gpu the command to draw these sprites, but if we do it in the execute function
			 * each Archetype will causes a draw call which is not optimal, so we just apply animation and transforms in the execute function
			 * and issue the draw call here so we can render all sprites in 1 draw call
			 *
			 * If you don't override this function it will call the execute function with all required parameters automatically thanks to template magic!
			 */
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::Context& context) override;

		private:
			static constexpr uint32_t NUM_QUADS_IN_BATCH = 10240;

			AssetHandle<Rendering::Material> _material;
			std::unique_ptr<Rendering::Model> _model;

			static float FastFmod(float a, float b);

			struct TextureData
			{
				glm::vec3             PageIndexAndSize{};
				alignas(16) glm::vec4 UVs[4];
			};

			struct TextureStore
			{
				std::array<TextureData, 10240> Textures;
			};

			struct ObjectBuffer
			{
				std::array<glm::vec4, 2'048'000> positions;
				std::array<uint32_t, 2'048'000>  textureIDs;
				uint32_t                         numObjects;
			};

			std::vector<std::unique_ptr<Rendering::Texture2D>> _texturePages;

			std::ranges::iota_view<size_t, size_t> _indexes;
	};;
} // REA
