#include "REA/System/PixelGridRenderer.hpp"

#include <execution>
#include <glm/gtc/random.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Shader.hpp>

#include "REA/PixelType.hpp"

namespace REA::System
{
	PixelGridRenderer::PixelGridRenderer(AssetHandle<Rendering::Material> material) :
		_material(material)
	{
		SSBO_GridInfo* tileData = _material->GetShader()->GetProperties().GetBufferData<SSBO_GridInfo>(0);

		tileData->colorLookup[PixelType::Air]   = Color(0x5890FFFF);
		tileData->colorLookup[PixelType::Sand]  = Color(0x9F944BFF);
		tileData->colorLookup[PixelType::Water] = Color(0x84BCFFFF);
		tileData->colorLookup[PixelType::Wood]  = Color(0x775937FF);
		tileData->colorLookup[PixelType::Void]  = Color(0x000000FF);

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

	void PixelGridRenderer::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		vk::CommandBuffer commandBuffer = contextProvider.GetContext<RenderingContext>()->Renderer->GetCommandBuffer().GetVkCommandBuffer();
		_material->GetShader()->BindGlobal(commandBuffer);

		_material->GetShader()->Update();

		_material->GetShader()->Bind(commandBuffer);

		for (int i = 0; i < entities.size(); ++i)
		{
			const Component::PixelGrid& pixelGrid = pixelGrids[i];
			SSBO_GridInfo*              gridInfo  = _material->GetShader()->GetProperties().GetBufferData<SSBO_GridInfo>(0);

			gridInfo->width           = pixelGrid.Width;
			gridInfo->height          = pixelGrid.Height;
			gridInfo->zoom            = pixelGrid.Zoom;
			gridInfo->offset          = pixelGrid.Offset;
			gridInfo->pointerPosition = pixelGrid.PointerPosition;

			//pixelGrid.Pixels

			//memcpy(&gridData->tileIDs[0], &pixelGrid.Pixels[0], sizeof(SSBO_GridData));

			_material->Update();

			_material->Bind(commandBuffer);

			_model->Bind(commandBuffer);

			commandBuffer.drawIndexed(_model->GetModelBuffer().GetBufferElementNum(1), 1, 0, 0, 0);
		}
	}
}
