#pragma once
#include <array>
#include <cstdint>

#include "SplitEngine/DataStructures.hpp"

using namespace SplitEngine;

namespace REA
{
	enum PixelFlags : uint8_t
	{
		Solid   = 1 << 0,
		Gravity = 1 << 1,
	};

	struct Pixel
	{
		typedef uint8_t ID;

		ID              PixelID         = 0;
		BitSet<uint8_t> Flags           = BitSet<uint8_t>(Gravity);
		uint8_t         Density         = 0;
		uint8_t         SpreadingFactor = 0;
	};

	inline std::array<Pixel, std::numeric_limits<Pixel::ID>::max()+1> Pixels;
}
