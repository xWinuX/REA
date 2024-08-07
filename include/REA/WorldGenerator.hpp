#pragma once

#include "Pixel.hpp"
#include "Component/PixelGrid.hpp"
#define FASTNOISE_STATIC_LIB
#include <FastNoise/FastNoise.h>


namespace REA
{
	class WorldGenerator
	{
		public:
			struct GenerationSettings
			{
				float TerrainBaseline          = 2000.0f;
				float TerrainRange             = 500.0f;
				int   GrassHeight              = 5;
				float CaveRange                = 100.0f;
				float CaveNoiseTreshold        = 0.5f;
				float CaveNoiseTresholdTerrain = 0.75f;
				float CaveNoiseFrequency       = 0.1f;
				float OverworldNoiseFrequency  = 0.05f;
			};

			struct TreeGenerationSettings
			{
				uint32_t Length            = 500; // Length of Branch (can be cut short if it collides with another)
				uint32_t StartingThickness = 50;  // Starting Thickness of Branch
				uint32_t MinThickness      = 3;   // Minimum Thickness of branch

				float BranchingStart  = 0.25f; // Percentage of branch
				float BranchingChance = 0.05f; // How likely branching is

				float StartingAngle  = 90.0f; // Starting angle of branch
				float AngleRange     = 45.0f; // Range of angle variation
				float AngleIncrement = 0.5f;  // How much the angle can increment on each step

				uint32_t LeadRadius = 100; // Radius of leaf circles
			};

		public:
			static void GenerateWorld(std::vector<Pixel::State>& world, Component::PixelGrid& pixelGrid, GenerationSettings& generationSettings);

		private:
			static glm::vec2 GenerateBranch(float                                   startX,
			                                float                                   startY,
			                                TreeGenerationSettings&                 treeGenerationSettings,
			                                size_t                                  numBranchIterations,
			                                size_t                                  currentIteration,
			                                bool                                    generateLeafs,
			                                FastNoise::SmartNode<FastNoise::White>& noiseGenerator,
			                                int                                     seed,
			                                std::vector<Pixel::State>&              world,
			                                Component::PixelGrid&                   pixelGrid);
	};
}
