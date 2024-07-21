#pragma once

#include <glm/vec2.hpp>
#include <SplitEngine/DataStructures.hpp>

#include "REA/Pixel.hpp"


namespace REA::Component
{
	struct PixelGrid
	{
		Pixel::State* PixelState = nullptr;

		int32_t Width  = 512;
		int32_t Height = 512;

		int32_t SimulationWidth  = 256;
		int32_t SimulationHeight = 256;

		glm::vec2 ViewTargetPosition = { 0.0f, 0.0f };

		uint64_t CameraEntityID = -1u;

		std::vector<Pixel>       PixelLookup{};
		std::vector<Pixel::Data> PixelDataLookup;
		std::vector<Color>       PixelColorLookup{};
	};
}
