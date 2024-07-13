#pragma once

#include "SplitEngine/DataStructures.hpp"

#include <limits>

using namespace SplitEngine;

namespace REA
{
	struct Pixel
	{
		typedef uint16_t ID;

		enum Flags : uint32_t
		{
			Static              = 1 << 0,
			RigidBodySpawn      = 1 << 1,
			Solid               = 1 << 7,
			Gravity             = 1 << 8,
			Electricity         = 1 << 9,
			ElectricityEmitter  = 1 << 10,
			ElectricityReceiver = 1 << 11,
		};

		/**
		 * State is typically used by shaders and should be kept as small as possible
		 */
		struct State
		{
			ID              PixelID = 0;
			uint8_t         Charge  = 0;
			BitSet<uint8_t> Flags   = BitSet<uint8_t>();

			float Temperature = 0.0f;

			struct
			{
				uint32_t RigidBodyID: 12 = 0u;
				uint32_t Index      : 20 = 0u;
			};
		};

		/**
		 * Data is used to lookup certain aspects of a pixel id.
		 * Stuff inside data will never change once its set
		 */
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

		std::string Name = "NAME_HERE";
		State       PixelState{};
	};

	struct SSBO_Pixels
	{
		Pixel::State Pixels[2097152];
	};
}
