#include "REA/System/SpriteRenderer.hpp"

#include <execution>
#include <imgui.h>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Rendering/Model.hpp>
#include <SplitEngine/Rendering/Texture2D.hpp>

#include "REA/Math.hpp"

namespace REA::System
{
	SpriteRenderer::SpriteRenderer(AssetHandle<Rendering::Material> material, Tools::ImagePacker::PackingData& packingData) :
		_material(material)
	{
		// Get buffers
		TextureStore* textureStore = _material->GetShader()->GetProperties().GetBufferData<TextureStore>(0);

		float limit = std::numeric_limits<float>::max();

		auto rawBuffer = _material->GetProperties().GetBufferRaw<ObjectBuffer>(0);

		glm::vec4 farAwayPos = glm::vec4(limit, limit, limit, 0.0f);

		for (int i = 0; i < Rendering::Vulkan::Device::MAX_FRAMES_IN_FLIGHT; ++i)
		{
			ObjectBuffer* b = reinterpret_cast<ObjectBuffer*>(rawBuffer[i]);
			for (auto& position: b->positions) { position = farAwayPos; }
		}

		// Prepare texture page info data for shader upload
		for (int i = 0; i < packingData.PackingInfos.size(); ++i)
		{
			Tools::ImagePacker::PackingInfo packingInfo = packingData.PackingInfos[i];
			textureStore->Textures[i].PageIndexAndSize  = { packingInfo.PageIndex, packingInfo.Size.x, packingInfo.Size.y };
			textureStore->Textures[i].UVs[0]            = { packingInfo.UVTopLeft, 0.0f, 0.0f };
			textureStore->Textures[i].UVs[1]            = { packingInfo.UVTopRight, 0.0f, 0.0f };
			textureStore->Textures[i].UVs[2]            = { packingInfo.UVBottomLeft, 0.0f, 0.0f };
			textureStore->Textures[i].UVs[3]            = { packingInfo.UVBottomRight, 0.0f, 0.0f };
		}

		// Actually create shader pages
		for (const auto& pageImage: packingData.PageImages)
		{
			_texturePages.push_back(std::make_unique<Rendering::Texture2D, Rendering::Texture2D::CreateInfo>({ pageImage, {} }));
		}

		// Send texture pages to shader
		_material->GetShader()->GetProperties().SetTextures(1, 0, _texturePages);

		const std::vector<uint32_t> quad        = { 0, 1, 2, 2, 1, 3 };
		const std::vector<uint16_t> quadIndices = { 0, 1, 2, 3, 4, 5 };

		std::vector<uint32_t> vertices;
		vertices.reserve(NUM_QUADS_IN_BATCH * 6);
		std::vector<uint16_t> indices;
		indices.reserve(NUM_QUADS_IN_BATCH * 6);
		for (size_t i = 0; i < NUM_QUADS_IN_BATCH; i++)
		{
			for (const uint32_t& vertex: quad) { vertices.push_back(vertex); }

			for (uint16_t index: quadIndices) { indices.push_back(index + (i * 6)); }
		}

		_model = std::make_unique<Rendering::Model, Rendering::Model::CreateInfo>({ reinterpret_cast<std::vector<std::byte>&>(vertices), indices });
	}

	void SpriteRenderer::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		size_t numEntities = 0;

		ObjectBuffer* objectBuffer = _material->GetProperties().GetBufferData<ObjectBuffer>(0);

		float deltaTime = contextProvider.GetContext<TimeContext>()->DeltaTime;

		for (const auto& archetype: archetypes)
		{
			Component::Transform*      transformComponents = archetype->GetComponents<Component::Transform>();
			Component::SpriteRenderer* spriteComponents    = archetype->GetComponents<Component::SpriteRenderer>();

			std::vector<uint64_t>& entities = archetype->Entities;

			memcpy(objectBuffer->positions.data() + numEntities, transformComponents, entities.size() * sizeof(glm::vec4));

			_indexes = std::ranges::iota_view(static_cast<size_t>(0), entities.size());

			std::for_each(std::execution::par_unseq,
			              _indexes.begin(),
			              _indexes.end(),
			              [this, spriteComponents, deltaTime, objectBuffer, numEntities](const size_t i)
			              {
				              Component::SpriteRenderer& spriteAnimatorComponent = spriteComponents[i];
				              SpriteTexture*             sprite                  = spriteAnimatorComponent.SpriteTexture.Get();

				              const float animationAdvance = spriteAnimatorComponent.AnimationSpeed * deltaTime;
				              const float newCurrentFrame  = spriteAnimatorComponent.CurrentFrame + animationAdvance;

				              spriteAnimatorComponent.CurrentFrame = Math::FastFmod(newCurrentFrame, static_cast<float>(sprite->_numSubSprites));

				              objectBuffer->textureIDs[numEntities + i] = sprite->GetTextureID(static_cast<uint32_t>(spriteAnimatorComponent.CurrentFrame));
			              });

			numEntities += entities.size();
		}

		objectBuffer->numObjects = numEntities;

		vk::CommandBuffer commandBuffer = contextProvider.GetContext<RenderingContext>()->Renderer->GetCommandBuffer().GetVkCommandBuffer();

		_material->GetShader()->BindGlobal(commandBuffer);

		_material->GetShader()->Update();

		_material->GetShader()->Bind(commandBuffer);

		_material->Update();

		_material->Bind(commandBuffer);

		_model->Bind(commandBuffer);

		const uint32_t numInstances = std::max(1u, static_cast<uint32_t>(std::ceil(static_cast<float>(numEntities) / static_cast<float>(NUM_QUADS_IN_BATCH))));
		commandBuffer.drawIndexed(_model->GetModelBuffer().GetBufferElementNum(1), numInstances, 0, 0, 0);
	}
}
