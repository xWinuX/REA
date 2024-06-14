#include "REA/System/PixelGridRenderer.hpp"

#include <execution>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Debug/Performance.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Shader.hpp>

namespace REA::System
{
	PixelGridRenderer::PixelGridRenderer(AssetHandle<Rendering::Material> material) :
		_material(material)
	{
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

	void PixelGridRenderer::Execute(Component::PixelGrid*         pixelGrids,
	                                Component::PixelGridRenderer* pixelGridRenderers,
	                                std::vector<uint64_t>&        entities,
	                                ECS::ContextProvider&         contextProvider,
	                                uint8_t                       stage)
	{
		vk::CommandBuffer commandBuffer = contextProvider.GetContext<RenderingContext>()->Renderer->GetCommandBuffer().GetVkCommandBuffer();
		_material->GetShader()->BindGlobal(commandBuffer);

		_material->GetShader()->Update();

		_material->GetShader()->Bind(commandBuffer);

		for (int i = 0; i < entities.size(); ++i)
		{
			Component::PixelGrid&               pixelGrid         = pixelGrids[i];
			const Component::PixelGridRenderer& pixelGridRenderer = pixelGridRenderers[i];
			SSBO_GridInfo*                      gridInfo          = _material->GetShader()->GetProperties().GetBufferData<SSBO_GridInfo>(0);

			gridInfo->width           = pixelGrid.Width;
			gridInfo->height          = pixelGrid.Height;
			gridInfo->zoom            = pixelGridRenderer.Zoom;
			gridInfo->offset          = pixelGridRenderer.Offset;
			gridInfo->pointerPosition = pixelGridRenderer.PointerPosition;
			gridInfo->renderMode      = pixelGridRenderer.RenderMode;
			size_t size               = std::min(pixelGrid.PixelColorLookup.size(), std::size(gridInfo->colorLookup));
			if (_sameGridCounter < Rendering::Vulkan::Device::MAX_FRAMES_IN_FLIGHT) { for (int i = 0; i < size; ++i) { gridInfo->colorLookup[i] = pixelGrid.PixelColorLookup[i]; } }


			Pixel::State* pixels = _material->GetProperties().GetBufferData<SSBO_Pixels>(0)->Pixels;

			pixelGrid.PixelState = pixels;

			//for (int i = 0; i < pixelGrid.ReadOnlyPixels.size(); ++i) { pixels[i].PixelID = pixelGrid.ReadOnlyPixels[i]; }

			_material->Update();

			_material->Bind(commandBuffer);

			_model->Bind(commandBuffer);

			commandBuffer.drawIndexed(_model->GetModelBuffer().GetBufferElementNum(1), 1, 0, 0, 0);

			uint64_t entity = entities[i];
			if (_previousEntity != entity) { _sameGridCounter = 0; }
			else { _sameGridCounter++; }
			_previousEntity = entity;
		}
	}
}
