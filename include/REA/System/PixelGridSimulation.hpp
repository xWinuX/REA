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

			PixelGridSimulation(const SimulationShaders& simulationShaders);


			AssetHandle<Rendering::Material> DebugMaterial;

		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;
			void Execute(Component::PixelGrid*              pixelGrids,
			             Component::Collider*               colliders,
			             std::vector<uint64_t>&             entities,
			             SplitEngine::ECS::ContextProvider& contextProvider,
			             uint8_t                            stage) override;

		private:
			struct RigidBody
			{
				uint32_t   ID        = -1u;
				glm::vec2  Position  = { 0, 0 };
				float      Rotation  = 0;
				uint32_t   DataIndex = -1u;
				glm::uvec2 Size      = { 0, 0 };
				uint32_t   _pad      = {};
			};

			struct Data
			{
				BitSet<uint32_t> Flags                        = BitSet<uint32_t>();
				uint32_t         Density                      = 0;
				uint32_t         SpreadingFactor              = 0;
				float            TemperatureResistance        = 0;
				float            BaseTemperature              = 0;
				float            LowerTemperatureLimit        = -273.15f;
				uint32_t         LowerTemperatureLimitPixelID = 0;
				float            HighTemperatureLimit         = 1'000'000;
				uint32_t         HighTemperatureLimitPixelID  = 0;
				float            TemperatureConversion        = 0.0f;
				uint32_t         BaseCharge                   = 0;
				float            ChargeAbsorbtionChance       = 0.0f;
			};

			struct SSBO_SimulationData
			{
				float     deltaTime = 0.0f;
				uint32_t  timer     = 0;
				float     rng       = 0.0f;
				uint32_t  width     = 0;
				uint32_t  height    = 0;
				Data      pixelLookup[1024];
				uint32_t  _pad[2] = {};
				RigidBody rigidBodies[100];
			};

			struct SSBO_MarchingCubes
			{
				uint32_t             numSegments = 0;
				alignas(8) glm::vec2 segments[100000];
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
				bool     Active   = false;
			};


			std::vector<RigidbodyEntry> _rigidBodyEntities{};
			std::vector<NewRigidBody>   _newRigidBodies{};

			MemoryHeap _rigidBodyDataHeap = MemoryHeap(1'000'000);

			AvailableStack<uint32_t> _availableRigidBodyIDs = AvailableStack<uint32_t>();

			uint32_t _rigidBodyIDCounter = 0;

			uint32_t _fif = 0;

			float _lineSimplificationTolerance = 50.0f;

			Rendering::Vulkan::CommandBuffer _commandBuffer;
			vk::Fence                        _computeFence;

			SimulationShaders _shaders;

			std::ranges::iota_view<size_t, size_t> _indexes;

			bool _paused = true;
			bool _doStep = false;

			uint32_t _readIndex  = 0;
			uint32_t _writeIndex = 1'000'000;

			bool _firstUpdate = true;

			Rendering::Vulkan::Buffer _vertexBuffer;

			b2AABB _cclRange;

			size_t _numLineSegements;

			void SwapPixelBuffer();

			void CmdWaitForPreviousComputeShader();

			static glm::uvec2 GetMargolusOffset(uint32_t frame);

			template<typename T>
			static T CeilDivide(T number, T divisor) { return (number + (divisor - 1)) / divisor; }
	};
}
