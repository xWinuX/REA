#include "REA/System/PixelGridSimulation.hpp"

#include <execution>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Vulkan/Device.hpp>

namespace REA::System
{
	PixelGridSimulation::PixelGridSimulation(AssetHandle<Rendering::Shader> pixelGridComputeShader):
		_shader(pixelGridComputeShader)
	{
		auto&                      properties = _shader->GetProperties();
		Rendering::Vulkan::Device* device     = _shader->GetPipeline().GetDevice();

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

		_shader->Update();

		SSBO_Pixels* inputPixels = _shader->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif);

		for (Pixel& pixel: inputPixels->Pixels)
		{
			pixel.PixelID = std::numeric_limits<int8_t>::max();
			pixel.Flags.Set(Gravity);
			pixel.Density         = std::numeric_limits<uint8_t>::min();
			pixel.SpreadingFactor = 0;
		}

		inputPixels->Pixels[500'550] = { static_cast<uint8_t>(1), BitSet<uint8_t>(Gravity), 3, 0 };

		// Set solid pixel
		Pixel solidPixel = { std::numeric_limits<Pixel::ID>::max(), BitSet<uint8_t>(Solid), std::numeric_limits<uint8_t>::min(), 0 };
		for (int i = 0; i < device->MAX_FRAMES_IN_FLIGHT; ++i) { _shader->GetProperties().GetBufferData<UBO_SimulationData>(0, i)->solidPixel = solidPixel; }


		vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
		_computeFence                       = _shader->GetPipeline().GetDevice()->GetVkDevice().createFence(fenceCreateInfo);
	}

	void PixelGridSimulation::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::Context& context)
	{
		Rendering::Vulkan::Device&     device       = context.Renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice();
		Rendering::Vulkan::QueueFamily computeQueue = context.Renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetQueueFamily(Rendering::Vulkan::QueueType::Compute);

		device.GetVkDevice().waitForFences(_computeFence, vk::True, UINT64_MAX);
		device.GetVkDevice().resetFences(_computeFence);


		UBO_SimulationData* simulationData = _shader->GetProperties().GetBufferData<UBO_SimulationData>(0, _fif);

		simulationData->deltaTime = context.DeltaTime;
		simulationData->timer++;

		SSBO_Pixels* inputPixels = _shader->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif);

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

		auto& properties = _shader->GetProperties();

		_commandBuffer.GetVkCommandBuffer().begin(commandBufferBeginInfo);

		_shader->Update();

		_shader->Bind(_commandBuffer.GetVkCommandBuffer(), _fif);

		_commandBuffer.GetVkCommandBuffer().dispatch(1'000'000 / 64, 1, 1);

		_commandBuffer.GetVkCommandBuffer().end();

		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers    = &_commandBuffer.GetVkCommandBuffer();

		computeQueue.GetVkQueue().submit(submitInfo, _computeFence);

		_fif = (_fif + 1) % device.MAX_FRAMES_IN_FLIGHT;

		Component::PixelGrid& pixelGrid = pixelGrids[0];
		pixelGrid.Pixels = _shader->GetProperties().GetBufferData<SSBO_Pixels>(1, _fif)->Pixels;
	}
}
