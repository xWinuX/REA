#pragma once

#include <glm/vec2.hpp>
#include <SplitEngine/DataStructures.hpp>

#include "REA/Pixel.hpp"

namespace REA::Component
{
	struct PixelGrid
	{
		std::array<PixelChunk, Constants::NUM_CHUNKS>* Chunks       = nullptr;
		std::vector<uint32_t>                          ChunkMapping = {};

		std::vector<std::vector<Pixel::State>> World{};

		glm::ivec2 ChunkOffset = { 0, 0 };

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
		std::vector<Pixel::Data> PixelDataLookup;
		std::vector<Color>       PixelColorLookup{};
	};
}
