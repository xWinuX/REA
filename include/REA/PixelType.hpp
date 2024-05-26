#pragma once
#include <cstdint>
#include <limits>

namespace REA
{
	enum PixelType : uint8_t
	{
		Air   = 0,
		Sand  = 1,
		Water = 2,
		Lava  = 3,
		Wood  = 4,
		Smoke = 5,
		Void  = 10,
	};
} // REA
