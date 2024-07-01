#pragma once
#include <ranges>
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Shader.hpp>
#include <SplitEngine/Rendering/Vulkan/CommandBuffer.hpp>

#include "REA/Component/Collider.hpp"
#include "REA/Component/PixelGrid.hpp"

namespace REA::System
{
	class PixelGridSimulation final : public ECS::System<Component::PixelGrid, Component::Collider>
	{
		public:
			struct SimulationShaders
			{
				AssetHandle<Rendering::Shader> IdleSimulation;
				AssetHandle<Rendering::Shader> FallingSimulation;
				AssetHandle<Rendering::Shader> AccumulateSimulation;
				AssetHandle<Rendering::Shader> MarchingSquareAlgorithm;
			};

			PixelGridSimulation(const SimulationShaders& simulationShaders);


			AssetHandle<Rendering::Material> DebugMaterial;

		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;
			void Execute(Component::PixelGrid* pixelGrids, Component::Collider* colliders, std::vector<uint64_t>& entities, SplitEngine::ECS::ContextProvider& contextProvider, uint8_t stage) override;

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

			struct SSBO_MarchingCubes
			{
				uint32_t             numSegments = 0;
				alignas(8) glm::vec2 segments[100000];
			};

			uint32_t _fif = 1;

			float _lineSimplificationTolerance = 0.5f;

			Rendering::Vulkan::CommandBuffer _commandBuffer;
			vk::Fence                        _computeFence;

			SimulationShaders _shaders;

			std::ranges::iota_view<size_t, size_t> _indexes;

			bool _paused = true;
			bool _doStep = false;

			bool _firstUpdate = true;

			Rendering::Vulkan::Buffer _vertexBuffer;

			std::vector<b2Fixture*> _staticFixtures;

			void CmdWaitForPreviousComputeShader();

			static glm::uvec2 GetMargolusOffset(uint32_t frame);

			template<typename T>
			static T CeilDivide(T number, T divisor) { return (number + (divisor - 1)) / divisor; }
	};
}
