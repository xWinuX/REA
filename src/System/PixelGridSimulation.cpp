#include "REA/System/PixelGridSimulation.hpp"

#include <execution>
#include <imgui.h>
#include <glm/gtc/random.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Debug/Performance.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Vulkan/Device.hpp>
#include <utility>

#include "IconsFontAwesome.h"
#include "REA/PixelType.hpp"
#include "REA/Context/ImGui.hpp"

namespace REA::System
{
	void PixelGridSimulation::ClearGrid(const Component::PixelGrid& pixelGrid)
	{
		SSBO_Pixels* inputPixels = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif);

		Pixel::State pixelState = pixelGrid.PixelLookup[_clearPixelID].PixelState;
		std::for_each(std::execution::par_unseq, std::begin(inputPixels->Pixels), std::end(inputPixels->Pixels), [this, pixelState](Pixel::State& pixel) { pixel = pixelState; });
	}

	PixelGridSimulation::PixelGridSimulation(const SimulationShaders& simulationShaders, Pixel::ID clearPixelID):
		_clearPixelID(clearPixelID),
		_shaders(simulationShaders)
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
		_shaders.IdleSimulation->Update();
		_shaders.AccumulateSimulation->Update();
		_shaders.AccumulateSimulation->Update();

		vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
		_computeFence                       = _shaders.FallingSimulation->GetPipeline().GetDevice()->GetVkDevice().createFence(fenceCreateInfo);
	}

	void PixelGridSimulation::CmdWaitForPreviousComputeShader()
	{
		vk::MemoryBarrier memoryBarrier[] = {
			vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderWrite),
			vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead),
			vk::MemoryBarrier(vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eShaderRead),
		};

		_commandBuffer.GetVkCommandBuffer().pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
		                                                    vk::PipelineStageFlagBits::eComputeShader,
		                                                    {},
		                                                    1,
		                                                    memoryBarrier,
		                                                    0,
		                                                    nullptr,
		                                                    0,
		                                                    nullptr);
	}

	void PixelGridSimulation::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		Context::ImGui* imGuiContext = contextProvider.GetContext<Context::ImGui>();
		ImGui::SetNextWindowDockID(imGuiContext->TopDockingID, ImGuiCond_Always);
		ImGui::Begin("Simulation");
		if (ImGui::Button(_paused ? ICON_FA_PLAY : ICON_FA_PAUSE)) { _paused = !_paused; }
		ImGui::SameLine();
		if (_paused) { if (ImGui::Button(ICON_FA_FORWARD_STEP)) { _doStep = true; } }
		if (ImGui::Button(ICON_FA_TRASH)) { _clearGrid = true; }
		ImGui::End();


		System<Component::PixelGrid>::ExecuteArchetypes(archetypes, contextProvider, stage);
	}

	glm::uvec2 PixelGridSimulation::GetMargolusOffset(uint32_t frame)
	{
		switch (frame % 4)
		{
			case 0:
				return { 0, 0 };
			case 1:
				return { 1, 0 };
			case 2:
				return { 0, 1 };
			case 3:
				return { 1, 1 };
			default:
				return { 0, 0 };
		}
	}

	void PixelGridSimulation::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		Rendering::Renderer* renderer = contextProvider.GetContext<RenderingContext>()->Renderer;

		Rendering::Vulkan::Device&     device       = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice();
		Rendering::Vulkan::QueueFamily computeQueue = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetQueueFamily(Rendering::Vulkan::QueueType::Compute);

		Component::PixelGrid& pixelGrid     = pixelGrids[0];
		size_t                numPixels     = pixelGrid.Width * pixelGrid.Height;
		size_t                numWorkgroups = CeilDivide(numPixels, 64ull);


		device.GetVkDevice().waitForFences(_computeFence, vk::True, UINT64_MAX);
		device.GetVkDevice().resetFences(_computeFence);

		if (_clearGrid)
		{
			ClearGrid(pixelGrid);
			_clearGrid = false;
		}

		uint32_t fif = _fif;

		UBO_SimulationData* simulationData = _shaders.FallingSimulation->GetProperties().GetBufferData<UBO_SimulationData>(0);
		simulationData->width  = pixelGrid.Width;
		simulationData->height = pixelGrid.Height;


		if (_firstUpdate)
		{
			memcpy(simulationData->pixelLookup, pixelGrid.PixelDataLookup.data(), pixelGrid.PixelDataLookup.size() * sizeof(Pixel::Data));
			_firstUpdate = false;
		}

		if (!_paused || _doStep)
		{

			simulationData->deltaTime = contextProvider.GetContext<TimeContext>()->DeltaTime;
			simulationData->timer = simulationData->timer+1;
			simulationData->rng    = glm::linearRand(0.0f, 1.0f);


			_commandBuffer.GetVkCommandBuffer().reset({});

			vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo({}, nullptr);

			_commandBuffer.GetVkCommandBuffer().begin(commandBufferBeginInfo);

			// Fall
			uint32_t flowIteration = 0;
			uint32_t   timer                 = simulationData->timer;
			glm::uvec2 margolusOffset        = GetMargolusOffset(timer);
			size_t     numWorkgroupsMorgulus = CeilDivide(CeilDivide(numPixels, 4ull), 64ull);


			_shaders.FallingSimulation->Update();

			for (int i = 0; i < 8; ++i)
			{
				CmdWaitForPreviousComputeShader();

				_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &flowIteration);
				_shaders.FallingSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 1, &margolusOffset);

				_shaders.FallingSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

				_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroupsMorgulus, 1, 1);

				fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;

				margolusOffset = GetMargolusOffset(timer + i);
				flowIteration++;
			}

			CmdWaitForPreviousComputeShader();

			_shaders.AccumulateSimulation->Update();

			_shaders.AccumulateSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

			_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

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

			_commandBuffer.GetVkCommandBuffer().dispatch(numWorkgroups, 1, 1);

			_commandBuffer.GetVkCommandBuffer().end();
		}

		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers    = &_commandBuffer.GetVkCommandBuffer();

		computeQueue.GetVkQueue().submit(submitInfo, _computeFence);

		_fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;

		pixelGrid.PixelState = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif)->Pixels;
	}
}
