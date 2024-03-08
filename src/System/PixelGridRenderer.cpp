#include "REA/System/PixelGridRenderer.hpp"

#include <glm/gtc/random.hpp>
#include <SplitEngine/Rendering/Shader.hpp>

namespace REA::System
{
	PixelGridRenderer::PixelGridRenderer(AssetHandle<Rendering::Material> material):
		_material(material)
	{
		GridBuffer* gridBuffer = _material->GetProperties().GetBuffer<GridBuffer>(0);

		for (uint32_t& tileID : gridBuffer->tileIDs)
		{
			tileID = glm::linearRand(0, 4);
		}

		TileData* tileData = _material->GetShader()->GetProperties().GetBuffer<TileData>(0);

		tileData->colorLookup[0] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
		tileData->colorLookup[1] = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
		tileData->colorLookup[2] = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
		tileData->colorLookup[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Black
		tileData->colorLookup[4] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // White

		const std::vector<uint32_t> quad        = { 0, 1, 2, 2, 1, 3 };
		const std::vector<uint16_t> quadIndices = { 0, 1, 2, 3, 4, 5 };

		std::vector<uint32_t> vertices;
		vertices.reserve(6);
		std::vector<uint16_t> indices;
		indices.reserve(6);
		for (size_t i = 0; i < 6; i++)
		{
			for (const uint32_t vertex: quad) { vertices.push_back(vertex); }

			for (const uint16_t index: quadIndices) { indices.push_back(index + (i * 6)); }
		}

		_model = std::make_unique<Rendering::Model, Rendering::Model::CreateInfo>({ reinterpret_cast<std::vector<std::byte>&>(vertices), indices });
	}

	void PixelGridRenderer::RunExecute(ECS::Context& context)
	{
		vk::CommandBuffer commandBuffer = context.RenderingContext->GetPhysicalDevice().GetDevice().GetCommandBuffer();

		_material->GetShader()->BindGlobal(commandBuffer);

		_material->GetShader()->Update();

		_material->GetShader()->Bind(commandBuffer);

		_material->Update();

		_material->Bind(commandBuffer);

		_model->Bind(commandBuffer);

		commandBuffer.drawIndexed(_model->GetModelBuffer().GetBufferElementNum(1), 1, 0, 0, 0);
	}
}
