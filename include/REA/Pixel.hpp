#pragma once
#include <cstdint>

#include "BitSet.hpp"

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
		typedef uint32_t ID;

		ID              PixelID         = 0;
		BitSet<uint8_t> Flags           = BitSet<uint8_t>(Gravity);
		int8_t          Density         = 0;
		uint8_t         SpreadingFactor = 0;
	};
}
