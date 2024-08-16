#pragma once

#include "SplitEngine/DataStructures.hpp"

#include <array>

#include "Constants.hpp"

using namespace SplitEngine;

namespace REA
{
	struct Pixel
	{
		typedef uint16_t ID;

		enum Flags : uint32_t
		{
			Solid = 1 << 0, // Non Solids can pass through eachother for example water can pass trough air but can't pass through sand
			Connected = 1 << 1, // Pixels with this set connect to each other and form rigidbodies
			Moving = 1 << 2, // Pixels with this set will not generate environment collisions when they are solid, this flag is managed by the shader and shouldn't be used
			Moved = 1 << 3, // Pixels with this set will not generate environment collisions when they are solid, this flag is managed by the shader and shouldn't be used
			Gravity = 1 << 8, // Pixels with gravity will fall
			Conductive = 1 << 9, // Can conduct Electricity
			ElectricityEmitter = 1 << 10, // Does not loose charge
			ElectricityReceiver = 1 << 11, // Consumes charge
		};

		enum Category
		{
			Static,
			Sand,
			Liquid,
			Electronic,
			Rigidbody,
			MAX_VALUE
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
			BitSet<uint32_t> FlagsCarryover               = BitSet<uint32_t>();
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
			uint32_t         Pulser                       = false;
			float            LowerChargeLimit             = -256;
			uint32_t         LowerChargeLimitPixelID      = 0;
			float            HighChargeLimit              = 256.0f;
			uint32_t         HighChargeLimitPixelID       = 0;
			float            Acidity                      = 0.0f;
			float            AcidityResistance            = 0.0f;
		};

		std::string Name = "NAME_HERE";
		State       PixelState{};
		Category    Cat = Category::Static;
	};

	typedef std::array<Pixel::State, Constants::NUM_ELEMENTS_IN_CHUNK> PixelChunk;
	typedef std::array<PixelChunk, Constants::NUM_CHUNKS>              PixelChunks;
	typedef std::array<uint32_t, Constants::NUM_CHUNKS>                PixelChunkMapping;

	struct SSBO_Pixels
	{
		PixelChunks Chunks;
	};

	struct SSBO_CopyPixels
	{
		PixelChunks Chunks;
	};
}
