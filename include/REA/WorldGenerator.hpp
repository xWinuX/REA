#pragma once

#include "Pixel.hpp"
#include "Component/PixelGrid.hpp"
#include "FastNoise/Generators/Generator.h"

namespace REA
{
	class WorldGenerator
	{
		public:
			struct GenerationSettings
			{
				float CaveNoiseTreshold       = 0.5f;
				float CaveNoiseFrequency      = 0.05f;
				float OverworldNoiseFrequency = 0.05f;
			};

			struct TreeGenerationSettings
			{
				size_t Length            = 100;
				size_t StartingThickness = 10;
				float  StartingAngle     = 0.0f;
				float  AngleRange        = 90.0f;
				float  AngleIncrement    = 1.0f;
			};

		public:
			static void GenerateWorld(std::vector<Pixel::State>& world, Component::PixelGrid& pixelGrid, GenerationSettings& generationSettings);

		private:
			static glm::vec2 GenerateBranch(size_t                     startX,
			                                size_t                     startY,
			                                TreeGenerationSettings&    treeGenerationSettings,
			                                size_t                     numBranchIterations,
			                                bool                       generateLeafs,
			                                std::vector<float>&        noise,
			                                std::vector<Pixel::State>& world,
			                                Component::PixelGrid&      pixelGrid);
	};
}
