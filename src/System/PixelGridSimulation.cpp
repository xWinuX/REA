#include "REA/System/PixelGridSimulation.hpp"

#include <execution>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Vulkan/Device.hpp>

namespace REA::System
{
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

		SSBO_Pixels* inputPixels = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif);

		for (Pixel& pixel: inputPixels->Pixels)
		{
			pixel.PixelID = std::numeric_limits<int8_t>::max();
			pixel.Flags.Set(Gravity);
			pixel.Density         = std::numeric_limits<uint8_t>::min();
			pixel.SpreadingFactor = 0;
		}

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

	void PixelGridSimulation::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::Context& context)
	{
		Rendering::Vulkan::Device&     device       = context.Renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice();
		Rendering::Vulkan::QueueFamily computeQueue = context.Renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetQueueFamily(Rendering::Vulkan::QueueType::Compute);

		device.GetVkDevice().waitForFences(_computeFence, vk::True, UINT64_MAX);
		device.GetVkDevice().resetFences(_computeFence);

		uint32_t fif = _fif;

		UBO_SimulationData* simulationData = _shaders.FallingSimulation->GetProperties().GetBufferData<UBO_SimulationData>(0);

		simulationData->deltaTime = context.DeltaTime;
		simulationData->timer++;
		simulationData->rng = glm::linearRand(0.0f, 1.0f);

		SSBO_Pixels* inputPixels = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(1, fif);

		if (Input::GetDown(KeyCode::SPACE))
		{
			for (int i = 0; i < 500; ++i)
			{
				if (i % 2 == 0) { continue; }
				inputPixels->Pixels[500'250 + i] = { static_cast<uint8_t>(1), BitSet<uint8_t>(Gravity), 3, 0 };
			}
		}

		_commandBuffer.GetVkCommandBuffer().reset({});

		constexpr vk::CommandBufferBeginInfo commandBufferBeginInfo = vk::CommandBufferBeginInfo({}, nullptr);

		auto& fallProperties = _shaders.FallingSimulation->GetProperties();

		_commandBuffer.GetVkCommandBuffer().begin(commandBufferBeginInfo);

		// Fall
		_shaders.FallingSimulation->Update();

		_shaders.FallingSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

		_commandBuffer.GetVkCommandBuffer().dispatch(1'000'000 / 64, 1, 1);

		// Flow
		uint32_t flowIteration = 0;

		_shaders.FlowSimulation->Update();

		for (int i = 0; i < 4; ++i)
		{
			CmdWaitForPreviousComputeShader(fif);
			fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;

			_shaders.FlowSimulation->PushConstant(_commandBuffer.GetVkCommandBuffer(), Rendering::ShaderType::Compute, 0, &flowIteration);

			_shaders.FlowSimulation->Bind(_commandBuffer.GetVkCommandBuffer(), fif);

			_commandBuffer.GetVkCommandBuffer().dispatch(1'000'000 / 64, 1, 1);

			flowIteration++;
		}

		_commandBuffer.GetVkCommandBuffer().end();

		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers    = &_commandBuffer.GetVkCommandBuffer();

		computeQueue.GetVkQueue().submit(submitInfo, _computeFence);

		_fif = (fif + 1) % device.MAX_FRAMES_IN_FLIGHT;

		Component::PixelGrid& pixelGrid = pixelGrids[0];
		pixelGrid.Pixels                = _shaders.FallingSimulation->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif)->Pixels;
	}
}
