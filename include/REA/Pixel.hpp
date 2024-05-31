#pragma once
#include <array>
#include <cstdint>

#include "Color.hpp"
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

		struct Data
		{
			ID              PixelID         = 0;
			BitSet<uint8_t> Flags           = BitSet<uint8_t>();
			uint8_t         Density         = 0;
			uint8_t         SpreadingFactor = 0;
		};

		std::string Name    = "NAME_HERE";
		Color       Color{};
		Data        PixelData{};
	};
}
