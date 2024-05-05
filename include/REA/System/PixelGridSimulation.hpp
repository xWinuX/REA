#pragma once
#include <ranges>
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Shader.hpp>
#include <SplitEngine/Rendering/Vulkan/CommandBuffer.hpp>

#include "REA/Component/PixelGrid.hpp"

namespace REA::System
{
	class PixelGridSimulation final : public ECS::System<Component::PixelGrid>
	{
		public:
			struct SimulationShaders
			{
				AssetHandle<Rendering::Shader> FallingSimulation;
				AssetHandle<Rendering::Shader> FlowSimulation;
			};

			PixelGridSimulation(SimulationShaders simulationShaders);
			void CmdWaitForPreviousComputeShader(uint32_t fif);

			void Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, SplitEngine::ECS::Context& context) override;

		private:
			struct UBO_SimulationData
			{
				float    deltaTime = 0.0f;
				uint32_t timer     = 0;
				float    rng       = 0.0f;
				Pixel    solidPixel{};
			};

			struct SSBO_Pixels
			{
				Pixel Pixels[1'000'000];
			};

			uint32_t _fif = 1;

			Rendering::Vulkan::CommandBuffer _commandBuffer;
			vk::Fence                        _computeFence;

			SimulationShaders _shaders;
	};
}
