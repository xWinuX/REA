#pragma once
#include <ranges>
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Shader.hpp>
#include <SplitEngine/Rendering/Vulkan/CommandBuffer.hpp>

#include "ReaSystem.hpp"
#include "REA/MemoryHeap.hpp"
#include "REA/WorldGenerator.hpp"
#include "REA/Component/Collider.hpp"
#include "REA/Component/PixelGrid.hpp"

namespace REA::System
{
	class PixelGridSimulation final : public ReaSystem<Component::PixelGrid>
	{
		public:
			struct SimulationShaders
			{
				AssetHandle<Rendering::Shader> IdleSimulation;
				AssetHandle<Rendering::Shader> RigidBodySimulation;
				AssetHandle<Rendering::Shader> RigidBodyRemove;
				AssetHandle<Rendering::Shader> FallingSimulation;
				AssetHandle<Rendering::Shader> AccumulateSimulation;
				AssetHandle<Rendering::Shader> PixelParticle;
				AssetHandle<Rendering::Shader> MarchingSquareAlgorithm;
				AssetHandle<Rendering::Shader> CCLInitialize;
				AssetHandle<Rendering::Shader> CCLColumn;
				AssetHandle<Rendering::Shader> CCLMerge;
				AssetHandle<Rendering::Shader> CCLRelabel;
				AssetHandle<Rendering::Shader> CCLExtract;
			};

			PixelGridSimulation(const SimulationShaders& simulationShaders, ECS::ContextProvider& contextProvider);


			AssetHandle<Rendering::Material> DebugMaterial;

		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;
			void Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage) override;


			void Destroy(ECS::ContextProvider& contextProvider) override;

		private:
			struct RigidBody
			{
				uint32_t   ID                 = -1u;
				uint32_t   DataIndex          = -1u;
				bool       NeedsRecalculation = false;
				float      Rotation           = 0;
				uint32_t   NumPixels          = 0;
				uint32_t   _pad               = 0;
				glm::vec2  Position           = { 0, 0 };
				b2Vec2     Velocity           = { 0, 0 };
				glm::ivec2 CounterVelocity    = { 0, 0 };
				glm::uvec2 Size               = { 0, 0 };
			};

			struct MarchingSquareWorld
			{
				uint32_t                                                 numSegments = 0;
				uint32_t                                                 _pad{};
				std::array<glm::vec2, Constants::MAX_WORLD_SEGMENTS * 2> segments{};
			};

			struct MarchingSquareChunk
			{
				uint32_t                                                 numSegments = 0;
				uint32_t                                                 _pad{};
				std::array<glm::vec2, Constants::MAX_CHUNK_SEGMENTS * 2> segments{};
			};

			struct SSBO_SimulationData
			{
				float             deltaTime   = 0.0f;
				uint32_t          timer       = 0;
				float             rng         = 0.0f;
				uint32_t          _pad        = 0;
				glm::ivec2        chunkOffset = { 0.0f, 0.0f };
				PixelChunkMapping chunkMapping;
				Pixel::Data       pixelLookup[1024];
			};

			struct SSBO_MarchingCubes
			{
				MarchingSquareWorld                                    connectedChunk;
				std::array<MarchingSquareChunk, Constants::NUM_CHUNKS> worldChunks;
			};

			struct SSBO_RigidBodyData
			{
				RigidBody rigidBodies[1024];
			};

			struct SSBO_Updates
			{
				uint32_t regenerateChunks[Constants::NUM_CHUNKS];
			};

			struct WorldGenerationSettings
			{
				float CaveNoiseTreshold       = 0.5f;
				float CaveNoiseFrequency      = 0.001f;
				float OverworldNoiseFrequency = 0.0005f;
			};

			uint32_t _fif = 1;

			float _lineSimplificationTolerance = 5.0f;

			Rendering::Vulkan::CommandBuffer _commandBuffer;
			vk::Fence                        _computeFence;

			SimulationShaders _shaders;

			std::ranges::iota_view<size_t, size_t> _indexes;

			MemoryHeap _rigidBodyDataHeap = MemoryHeap(Constants::NUM_ELEMENTS_X * Constants::NUM_ELEMENTS_Y);

			bool _paused = true;
			bool _doStep = false;

			bool _firstUpdate = true;

			std::vector<Pixel::State> _world{};

			Rendering::Vulkan::Buffer _vertexBuffer;

			Rendering::Vulkan::Buffer _copyBuffer;

			b2AABB _cclRange = { { 10'000'000, 10'000'000 }, { 0, 0 } };

			size_t _numLineSegements = 0;

			void CmdWaitForPreviousComputeShader();

			static glm::uvec2 GetMargolusOffset(uint32_t frame);

			template<typename T>
			static T CeilDivide(T number, T divisor) { return (number + (divisor - 1)) / divisor; }
	};
}
