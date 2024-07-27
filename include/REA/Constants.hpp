#pragma once
#include <cstdint>

namespace REA
{
	namespace Constants
	{
		constexpr uint32_t CHUNKS_X   = 8;
		constexpr uint32_t CHUNKS_Y   = 8;
		constexpr uint32_t CHUNK_SIZE = 128;

		constexpr uint32_t NUM_CHUNKS = CHUNKS_X * CHUNKS_Y;
		constexpr uint32_t NUM_ELEMENTS_IN_CHUNK = CHUNK_SIZE * CHUNK_SIZE;

		constexpr uint32_t NUM_ELEMENTS_X = CHUNKS_X * CHUNK_SIZE;
		constexpr uint32_t NUM_ELEMENTS_Y = CHUNKS_Y * CHUNK_SIZE;
	}
}
