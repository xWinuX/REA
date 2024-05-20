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

		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;

		private:
			static constexpr uint32_t NUM_QUADS_IN_BATCH = 10240;

			AssetHandle<Rendering::Material>  _material;
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
