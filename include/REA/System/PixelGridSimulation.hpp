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
				AssetHandle<Rendering::Shader> AccumulateSimulation;
			};

			void ClearGrid(const Component::PixelGrid& pixelGrid);
			PixelGridSimulation(const SimulationShaders& simulationShaders, Pixel::ID clearPixelID);

		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;
			void Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, SplitEngine::ECS::ContextProvider& contextProvider, uint8_t stage) override;

		private:
			struct UBO_SimulationData
			{
				float       deltaTime = 0.0f;
				uint32_t    timer     = 0;
				float       rng       = 0.0f;
				uint32_t    width     = 0;
				uint32_t    height    = 0;
				Pixel::Data pixelLookup[1024];
			};

			struct SSBO_Pixels
			{
				Pixel::State Pixels[1000000];
			};

			Pixel::ID _clearPixelID;

			uint32_t _fif = 1;

			Rendering::Vulkan::CommandBuffer _commandBuffer;
			vk::Fence                        _computeFence;

			SimulationShaders _shaders;

			bool _paused    = true;
			bool _clearGrid = true;
			bool _doStep    = false;

			bool _firstUpdate = true;

			void CmdWaitForPreviousComputeShader();

			static glm::uvec2 GetMargolusOffset(uint32_t frame);

			template<typename T>
			static T CeilDivide(T number, T divisor) { return (number + (divisor - 1)) / divisor; }
	};
}
