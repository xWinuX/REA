#pragma once
#include <ranges>
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Shader.hpp>
#include <SplitEngine/Rendering/Vulkan/CommandBuffer.hpp>

#include "REA/MemoryHeap.hpp"
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
				AssetHandle<Rendering::Shader> RigidBodySimulation;
				AssetHandle<Rendering::Shader> RigidBodyRemove;
				AssetHandle<Rendering::Shader> FallingSimulation;
				AssetHandle<Rendering::Shader> AccumulateSimulation;
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
			void Execute(Component::PixelGrid*  pixelGrids,
			             Component::Collider*   colliders,
			             std::vector<uint64_t>& entities,
			             ECS::ContextProvider&  contextProvider,
			             uint8_t                stage) override;

		private:
			struct RigidBody
			{
				uint32_t   ID                 = -1u;
				uint32_t   DataIndex          = -1u;
				bool       NeedsRecalculation = false;
				float      Rotation           = 0;
				glm::vec2  Position           = { 0, 0 };
				glm::uvec2 Size               = { 0, 0 };
			};

			struct SSBO_SimulationData
			{
				float       deltaTime        = 0.0f;
				uint32_t    timer            = 0;
				float       rng              = 0.0f;
				uint32_t    width            = 0;
				uint32_t    height           = 0;
				uint32_t    simulationWidth  = 0;
				uint32_t    simulationHeight = 0;
				uint32_t    _pad             = 0;
				glm::vec2   targetPosition   = { 0.0f, 0.0f };
				Pixel::Data pixelLookup[1024];
			};

			struct SSBO_MarchingCubes
			{
				uint32_t  numConnectedSegments = 0;
				uint32_t  numSolidSegments     = 0;
				glm::vec2 connectedSegments[100000];
				glm::vec2 solidSegments[100000];
			};

			struct SSBO_RigidBodyData
			{
				RigidBody rigidBodies[1024];
			};


			struct NewRigidBody
			{
				glm::uvec2 Offset;
				glm::uvec2 Size;
				glm::uvec2 SeedPoint;
				uint32_t   RigidBodyID;
			};

			struct RigidbodyEntry
			{
				uint64_t EntityID = -1u;
				bool     Enabled  = false;
				bool     Active   = false;
			};

			struct WorldGenerationSettings
			{
				float CaveNoiseTreshold  = 0.5f;
				float CaveNoiseFrequency = 0.01f;
				float OverworldNoiseFrequency = 0.005f;
			};

			std::vector<RigidbodyEntry> _rigidBodyEntities{};
			std::vector<NewRigidBody>   _newRigidBodies{};
			std::vector<uint32_t>       _deleteRigidbody{};

			MemoryHeap _rigidBodyDataHeap = MemoryHeap(1048576);

			AvailableStack<uint32_t> _availableRigidBodyIDs = AvailableStack<uint32_t>();

			uint64_t _staticEnvironmentEntityID;

			uint32_t _rigidBodyIDCounter = 1;

			uint32_t _fif = 0;

			float _lineSimplificationTolerance = 0.5f;

			Rendering::Vulkan::CommandBuffer _commandBuffer;
			vk::Fence                        _computeFence;

			SimulationShaders _shaders;

			std::ranges::iota_view<size_t, size_t> _indexes;

			bool _paused        = true;
			bool _doStep        = false;
			bool _generateWorld = true;


			WorldGenerationSettings _worldGenerationSettings {};

			uint32_t _readIndex  = 0;
			uint32_t _writeIndex = 8388608;

			bool _firstUpdate = true;

			std::vector<Pixel::State> _world{};

			Rendering::Vulkan::Buffer _vertexBuffer;

			b2AABB _cclRange = { { 10'000'000, 10'000'000 }, { 0, 0 } };

			size_t _numLineSegements = 0;

			void SwapPixelBuffer();

			void CmdWaitForPreviousComputeShader();

			static glm::uvec2 GetMargolusOffset(uint32_t frame);

			template<typename T>
			static T CeilDivide(T number, T divisor) { return (number + (divisor - 1)) / divisor; }
	};
}
