#pragma once

#include <box2d/b2_fixture.h>
#include <glm/vec2.hpp>
#include <SplitEngine/DataStructures.hpp>
#include <REA/MemoryHeap.hpp>

#include "REA/Pixel.hpp"

namespace REA::Component
{
	struct PixelGrid
	{
		typedef std::array<Pixel::State, Constants::NUM_ELEMENTS_IN_CHUNK> PixelChunk;

		struct RigidBody
		{
			uint32_t   ID                 = -1u;
			uint32_t   DataIndex          = -1u;
			bool       NeedsRecalculation = false;
			float      Rotation           = 0;
			glm::vec2  Position           = { 0, 0 };
			glm::uvec2 Size               = { 0, 0 };
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

		std::array<PixelChunk, Constants::NUM_CHUNKS>* Chunks        = nullptr;
		std::vector<std::vector<b2Fixture*>>           ChunkFixtures = std::vector<std::vector<b2Fixture*>>(Constants::NUM_CHUNKS);
		std::vector<uint32_t>                          ChunkMapping{};
		std::vector<uint32_t>                          ChunkRegenerate{};

		uint64_t                    _staticEnvironmentEntityID = -1;
		uint32_t                    RigidBodyIDCounter         = 1;
		AvailableStack<uint32_t>    AvailableRigidBodyIDs{};
		std::vector<RigidbodyEntry> RigidBodyEntities{};
		std::vector<NewRigidBody>   NewRigidBodies{};
		std::vector<uint32_t>       DeleteRigidbody{};

		std::vector<std::vector<Pixel::State>> World{};

		glm::ivec2 ChunkOffset = { 0, 0 };

		glm::ivec2 PreviousChunkOffset = { 0, 0 };

		int32_t WorldChunksX = 16;
		int32_t WorldChunksY = 16;

		int32_t SimulationChunksX = 8;
		int32_t SimulationChunksY = 8;

		int32_t WorldWidth  = 1024;
		int32_t WorldHeight = 1024;

		int32_t SimulationWidth  = 1024;
		int32_t SimulationHeight = 1024;

		uint64_t CameraEntityID = -1u;

		std::vector<Pixel>       PixelLookup{};
		std::vector<Pixel::Data> PixelDataLookup{};
		std::vector<Color>       PixelColorLookup{};
	};
}
