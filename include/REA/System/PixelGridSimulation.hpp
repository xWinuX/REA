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
				AssetHandle<Rendering::Shader> IdleSimulation;
				AssetHandle<Rendering::Shader> FallingSimulation;
				AssetHandle<Rendering::Shader> FlowSimulation;
			};

			void ClearGrid();
			PixelGridSimulation(const SimulationShaders& simulationShaders, Pixel clearPixel);

		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;
			void Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, SplitEngine::ECS::ContextProvider& contextProvider, uint8_t stage) override;

		private:
			struct UBO_SimulationData
			{
				float    deltaTime = 0.0f;
				uint32_t timer     = 0;
				float    rng       = 0.0f;
				uint32_t width     = 0;
				uint32_t height    = 0;
			};

			struct SSBO_Pixels
			{
				Pixel::Data Pixels[1000000];
			};

			Pixel _clearPixel;

			uint32_t _fif = 1;

			Rendering::Vulkan::CommandBuffer _commandBuffer;
			vk::Fence                        _computeFence;

			SimulationShaders _shaders;

			bool _paused    = true;
			bool _clearGrid = false;
			bool _doStep    = false;

			void CmdWaitForPreviousComputeShader();

			static glm::uvec2 GetMargolusOffset(uint32_t frame);

			template<typename T>
			static T CeilDivide(T number, T divisor) { return (number + (divisor - 1)) / divisor; }
	};
}
