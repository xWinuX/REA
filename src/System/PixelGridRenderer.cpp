#include "REA/System/PixelGridRenderer.hpp"

#include <execution>
#include <glm/gtc/random.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Shader.hpp>

namespace REA::System
{
	PixelGridRenderer::PixelGridRenderer(AssetHandle<Rendering::Material> material) :
		_material(material)
	{
		SSBO_GridInfo* tileData = _material->GetShader()->GetProperties().GetBufferData<SSBO_GridInfo>(0);

		tileData->colorLookup[0] = glm::vec4((1.0f/255.0f) * 0x07, (1.0f/255.0f) * 0x1F, (1.0f/255.0f) * 0x16, 1.0f); // Black
		tileData->colorLookup[1] = glm::vec4((1.0f/255.0f) * 0xE6, (1.0f/255.0f) * 0xB0, (1.0f/255.0f) * 0x45, 1.0f); // Black
		tileData->colorLookup[2] = glm::vec4((1.0f/255.0f) * 0x26, (1.0f/255.0f) * 0x4E, (1.0f/255.0f) * 0x80, 1.0f); // White
		tileData->colorLookup[3] = glm::vec4((1.0f/255.0f) * 0x55, (1.0f/255.0f) * 0x55, (1.0f/255.0f) * 0x55, 1.0f); // White
		//tileData->colorLookup[2] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Green
		//tileData->colorLookup[1] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Blue
		//tileData->colorLookup[0] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // White
		//tileData->colorLookup[4] = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // White
		//tileData->colorLookup[5] = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // White
		//tileData->colorLookup[6] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // White
		//tileData->colorLookup[7] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Black

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

	void PixelGridRenderer::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::Context& context)
	{
		vk::CommandBuffer commandBuffer = context.Renderer->GetCommandBuffer().GetVkCommandBuffer();
		_material->GetShader()->BindGlobal(commandBuffer);

		_material->GetShader()->Update();

		_material->GetShader()->Bind(commandBuffer);

		for (int i = 0; i < entities.size(); ++i)
		{
			const Component::PixelGrid& pixelGrid = pixelGrids[i];
			SSBO_GridInfo* tileData = _material->GetShader()->GetProperties().GetBufferData<SSBO_GridInfo>(0);

			tileData->width  = pixelGrid.Width;
			tileData->height = pixelGrid.Height;

			//pixelGrid.Pixels

			//memcpy(&gridData->tileIDs[0], &pixelGrid.Pixels[0], sizeof(SSBO_GridData));

			_material->Update();

			_material->Bind(commandBuffer);

			_model->Bind(commandBuffer);

			commandBuffer.drawIndexed(_model->GetModelBuffer().GetBufferElementNum(1), 1, 0, 0, 0);
		}
	}
}
