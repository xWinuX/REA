#include "REA/System/PixelGridSimulation.hpp"

#include <execution>
#include <imgui.h>
#include <glm/gtc/random.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Vulkan/Device.hpp>

#include "IconsFontAwesome.h"

namespace REA::System
{
	void PixelGridSimulation::ClearGrid()
	{
		SSBO_Pixels* inputPixels = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif);

		std::for_each(std::execution::par_unseq,
		              std::begin(inputPixels->Pixels),
		              std::end(inputPixels->Pixels),
		              [](Pixel& pixel)
		              {
			              pixel.PixelID = std::numeric_limits<int8_t>::max();
			              pixel.Flags.Set(Gravity);
			              pixel.Density         = std::numeric_limits<uint8_t>::min();
			              pixel.SpreadingFactor = 0;
		              });
	}

	PixelGridSimulation::PixelGridSimulation(SimulationShaders shaders):
		_shaders(shaders)
	{
		auto&                      properties = _shaders.FallingSimulation->GetProperties();
		Rendering::Vulkan::Device* device     = _shaders.FallingSimulation->GetPipeline().GetDevice();

		_commandBuffer = std::move(device->GetQueueFamily(Rendering::Vulkan::QueueType::Compute).AllocateCommandBuffer(Rendering::Vulkan::QueueType::Compute));

		_commandBuffer.GetVkCommandBufferRaw().SetFramePtr(&_fif);

		Rendering::Vulkan::Buffer& writeBuffer = properties.GetBuffer(2);

		for (int i = 0; i < device->MAX_FRAMES_IN_FLIGHT; ++i)
		{
			uint32_t fifIndex    = (i + 1) % device->MAX_FRAMES_IN_FLIGHT;
			auto&    bufferInfos = properties.GetBufferInfo(2, fifIndex);
			properties.SetBuffer(1, writeBuffer, bufferInfos.offset, bufferInfos.range, i);
		}

		//	properties.OverrideBufferPtrs(1, writeBuffer);

		_shaders.FallingSimulation->Update();
		_shaders.FlowSimulation->Update();

		ClearGrid();

		// Set solid pixel
		Pixel solidPixel = { std::numeric_limits<Pixel::ID>::max(), BitSet<uint8_t>(Solid), std::numeric_limits<uint8_t>::min(), 0 };
		_shaders.FallingSimulation->GetProperties().GetBufferData<UBO_SimulationData>(0)->solidPixel = solidPixel;

		vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
		_computeFence                       = _shaders.FallingSimulation->GetPipeline().GetDevice()->GetVkDevice().createFence(fenceCreateInfo);
	}

	void PixelGridSimulation::CmdWaitForPreviousComputeShader(uint32_t fif)
	{
		vk::MemoryBarrier memoryBarrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);

		SplitEngine::Rendering::Shader::Properties properties          = _shaders.FallingSimulation->GetProperties();
		auto                                       bufferInfos         = properties.GetBufferInfo(2, fif);
		vk::BufferMemoryBarrier                    bufferMemoryBarrier = vk::BufferMemoryBarrier(vk::AccessFlagBits::eShaderWrite,
		                                                                                         vk::AccessFlagBits::eShaderRead,
		                                                                                         2,
		                                                                                         2,
		                                                                                         properties.GetBuffer(2).GetVkBuffer(),
		                                                                                         bufferInfos.offset,
		                                                                                         bufferInfos.range);


		_commandBuffer.GetVkCommandBuffer().pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
		                                                    vk::PipelineStageFlagBits::eComputeShader,
		                                                    {},
		                                                    1,
		                                                    &memoryBarrier,
		                                                    1,
		                                                    &bufferMemoryBarrier,
		                                                    0,
		                                                    nullptr);
	}

	void PixelGridSimulation::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& context, uint8_t stage)
	{
		ImGui::Begin("Simulation");
		if (ImGui::Button(_paused ? ICON_FA_PLAY : ICON_FA_PAUSE)) { _paused = !_paused; }
		ImGui::SameLine();
		if (_paused) { if (ImGui::Button(ICON_FA_FORWARD_STEP)) { _doStep = true; } }
		if (ImGui::Button(ICON_FA_TRASH)) { ClearGrid(); }
		ImGui::End();


		System<Component::PixelGrid>::ExecuteArchetypes(archetypes, context, stage);
	}

	void PixelGridSimulation::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		Rendering::Renderer* renderer = contextProvider.GetContext<RenderingContext>()->Renderer;

		Rendering::Vulkan::Device&     device       = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice();
		Rendering::Vulkan::QueueFamily computeQueue = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetQueueFamily(Rendering::Vulkan::QueueType::Compute);

		device.GetVkDevice().waitForFences(_computeFence, vk::True, UINT64_MAX);
		device.GetVkDevice().resetFences(_computeFence);

		uint32_t fif = _fif;

		if (!_paused || _doStep)
		{
			UBO_SimulationData* simulationData = _shaders.FallingSimulation->GetProperties().GetBufferData<UBO_SimulationData>(0);

			simulationData->deltaTime = contextProvider.GetContext<TimeContext>()->DeltaTime;
			simulationData->timer++;
			simulationData->rng = glm::linearRand(0.0f, 1.0f);

			_commandBuffer.GetVkCommandBuffer().reset({});

			constexpr vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo({}, nullptr);

			_commandBuffer.GetVkCommandBuffer().begin(commandBufferBeginInfo);

			// Fall
			_shaders.FallingSimulation->Update();

			_shaders.FallingSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

			_commandBuffer.GetVkCommandBuffer().dispatch(1'000'000 / 64, 1, 1);

			// Flow
			uint32_t flowIteration = 0;

			_shaders.FlowSimulation->Update();

			for (int i = 0; i < 1; ++i)
			{
				CmdWaitForPreviousComputeShader(fif);
				fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;

				_shaders.FlowSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &flowIteration);

				_shaders.FlowSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

				_commandBuffer.GetVkCommandBuffer().dispatch(1'000'000 / 64, 1, 1);

				flowIteration++;
			}

			_commandBuffer.GetVkCommandBuffer().end();

			_doStep = false;
		}
		else
		{
			_commandBuffer.GetVkCommandBuffer().reset({});

			constexpr vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo({}, nullptr);

			_commandBuffer.GetVkCommandBuffer().begin(commandBufferBeginInfo);

			_shaders.IdleSimulation->Update();

			_shaders.IdleSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

			_commandBuffer.GetVkCommandBuffer().dispatch(1'000'000 / 64, 1, 1);

			_commandBuffer.GetVkCommandBuffer().end();
		}

		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers    = &_commandBuffer.GetVkCommandBuffer();

		computeQueue.GetVkQueue().submit(submitInfo, _computeFence);

		_fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;

		Component::PixelGrid& pixelGrid = pixelGrids[0];
		pixelGrid.Pixels                = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif)->Pixels;
	}
}
