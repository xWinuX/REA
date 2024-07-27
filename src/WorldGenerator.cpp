#include "REA/WorldGenerator.hpp"

#define FASTNOISE_STATIC_LIB
#include <FastNoise/FastNoise.h>

#include <glm/common.hpp>
#include <glm/gtc/random.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/KeyCode.hpp>

#include "REA/Math.hpp"
#include "REA/PixelType.hpp"
#include "REA/Component/PixelGrid.hpp"

#define GET_PIXEL(pixelType) pixelGrid.PixelLookup[pixelType].PixelState

namespace REA
{
	void WorldGenerator::GenerateWorld(std::vector<Pixel::State>& world, Component::PixelGrid& pixelGrid, GenerationSettings& generationSettings)
	{
		auto caveNoiseGenerator = FastNoise::NewFromEncodedNodeTree("IgAAAAA/j8J1Pg0AAwAAAB+FE0EQAM3MDEATAJqZGT8LAAAAAAAAAAAAAQAAAAQAAAAA9iicPwAfhes+AArXoz0ArkeNQQ==");
		std::vector<float> caveNoise = std::vector<float>(world.size());
		caveNoiseGenerator->GenUniformGrid2D(caveNoise.data(), 0, 0, pixelGrid.Width, pixelGrid.Height, 0.01f * generationSettings.CaveNoiseFrequency, 1234);

		auto overworldNoiseGenerator =
				FastNoise::NewFromEncodedNodeTree("IQATAClcDz4UACkAAAAAgD8AAACAPwAAAAAAAAAAAAAQAOxR+D8ZAA0AAwAAAD0KF0ApAAAAAAA/AHsUrj4AzczMPQCPwnU+AAAAAD8=");
		std::vector<float> overworldNoise = std::vector<float>(world.size());
		overworldNoiseGenerator->GenUniformGrid2D(overworldNoise.data(), 0, 0, pixelGrid.Width, pixelGrid.Height, 0.01f * generationSettings.OverworldNoiseFrequency, 1234);

		auto normalNoiseGenerator = FastNoise::NewFromEncodedNodeTree("IQATAClcDz4UACkAAAAAgD8AAACAPwAAAAAAAAAAAAAQAOxR+D8ZAA0AAwAAAD0KF0ApAAAAAAA/AHsUrj4AzczMPQCPwnU+AAAAAD8=");
		std::vector<float> normalNoise = std::vector<float>(world.size());
		normalNoiseGenerator->GenUniformGrid2D(normalNoise.data(), 0, 0, pixelGrid.Width, pixelGrid.Height, 0.1f, 1234);

		const float blendRegionHeight = 30.0f; // Height of the blending region
		const float caveLayerHeight   = pixelGrid.Height / 2;

		const float blendStart = caveLayerHeight;
		const float blendEnd   = caveLayerHeight + blendRegionHeight;

		const float waterLevelStart = caveLayerHeight + 100.0f;
		const float waterLevelEnd   = caveLayerHeight + 150.0f;

		// Basic generation of Terrain and caves
		for (int i = 0; i < world.size(); ++i)
		{
			size_t x = i % pixelGrid.Width;
			size_t y = i / pixelGrid.Width;

			if (y < caveLayerHeight) { if (caveNoise[i] < generationSettings.CaveNoiseTreshold) { world[i] = GET_PIXEL(PixelType::Stone); } }
			else
			{
				float noise         = glm::clamp(glm::abs(overworldNoise[x]) + 0.1f, 0.0f, 1.0f);
				float terrainHeight = (noise * (caveLayerHeight / 2.0f)) + caveLayerHeight;

				// Calculate blend factor
				float blendFactor = (y - blendStart) / (blendEnd - blendStart);
				blendFactor       = std::clamp(blendFactor, 0.0f, 1.0f);

				if (terrainHeight > y && caveNoise[i] < glm::mix(generationSettings.CaveNoiseTreshold, 0.80f, blendFactor))
				{
					if (y >= blendStart && y <= blendEnd)
					{
						float blendNoise = glm::abs(overworldNoise[x + (pixelGrid.Width * 100)]);
						world[i]         = blendStart + (blendNoise * blendRegionHeight) > y ? GET_PIXEL(PixelType::Stone) : GET_PIXEL(PixelType::Dirt);
					}
					else { world[i] = GET_PIXEL(PixelType::Dirt); }
				}
			}
		}


		// Grass
		uint32_t grassHeight = 3;
		for (int i = 0; i < grassHeight; ++i)
		{
			for (int x = 0; x < pixelGrid.Width; ++x)
			{
				for (int y = pixelGrid.Height - 1; y > 0; --y)
				{
					size_t index      = y * pixelGrid.Width + x;
					size_t belowIndex = (y - 1) * pixelGrid.Width + x;
					if (world[belowIndex].PixelID == PixelType::Dirt || world[belowIndex].PixelID == PixelType::Grass)
					{
						world[index] = GET_PIXEL(PixelType::Grass);
						break;
					}
				}
			}
		}

		// Trees
		/*
		float nextTreeEdge = 0;
		for (int x = 300; x < pixelGrid.Width - 300; ++x)
		{
			if (x > nextTreeEdge+(300))
			{*
				int x = 300;
				for (size_t y = pixelGrid.Height - 1; y > 0; --y)
				{
					size_t index      = y * pixelGrid.Width + x;
					size_t belowIndex = (y - 1) * pixelGrid.Width + x;
					if (world[belowIndex].PixelID == PixelType::Grass)
					{
						uint32_t treeHeight = 100;
						uint32_t treeWidth  = 10;
						float    xOffset    = 0;

						TreeGenerationSettings treeGenerationSettings{};

						// Tree and leafs
						treeGenerationSettings.Length            = 200;
						treeGenerationSettings.StartingThickness = 50;
						treeGenerationSettings.StartingAngle     = 90;
						treeGenerationSettings.AngleIncrement    = 0.5f;
						treeGenerationSettings.AngleRange        = 45;

						nextTreeEdge = GenerateBranch(x, y, treeGenerationSettings, 3, true, normalNoise, world, pixelGrid).x;

						LOG("next tree edge {0}", nextTreeEdge);

						// Roots
						treeGenerationSettings.Length            = 100;
						treeGenerationSettings.StartingThickness = 50;
						treeGenerationSettings.AngleIncrement    = 5.0f;
						treeGenerationSettings.AngleRange        = 45;
						treeGenerationSettings.StartingAngle     = -90;

						GenerateBranch(x, y, treeGenerationSettings, 5, false, normalNoise, world, pixelGrid);

						treeGenerationSettings.StartingThickness = 30;
						treeGenerationSettings.StartingAngle     = -90 - 45;
						GenerateBranch(x, y, treeGenerationSettings, 4, false, normalNoise, world, pixelGrid);

						treeGenerationSettings.StartingAngle = -90 + 45;
						GenerateBranch(x, y, treeGenerationSettings, 4, false, normalNoise, world, pixelGrid);

						break;
					}
				}*/
			//}
		//}
	}

	glm::vec2 WorldGenerator::GenerateBranch(size_t                     startX,
	                                         size_t                     startY,
	                                         TreeGenerationSettings&    treeGenerationSettings,
	                                         size_t                     numBranchIterations,
	                                         bool                       generateLeafs,
	                                         std::vector<float>&        noise,
	                                         std::vector<Pixel::State>& world,
	                                         Component::PixelGrid&      pixelGrid)
	{
		// Branches
		glm::vec2 maxPosition        = { startX, startY };
		glm::vec2 position           = { startX, startY };
		float     angle              = treeGenerationSettings.StartingAngle;
		int32_t   branchingCounter   = 0;
		int32_t   branchingDirection = glm::linearRand(0.0f, 1.0f) < 0.5f ? -1 : 1;
		for (int branchStep = 0; branchStep < treeGenerationSettings.Length; ++branchStep)
		{
			float branchProgress = static_cast<float>(branchStep) / static_cast<float>(treeGenerationSettings.Length);

			glm::vec2 dir = glm::abs(Math::VecFromAngle(angle + 90));

			uint32_t branchWidth  = static_cast<uint32_t>(glm::round(static_cast<float>(treeGenerationSettings.StartingThickness) * (1.0f - branchProgress)));
			uint32_t branchHeight = branchWidth;

			branchWidth  = glm::max(1u, static_cast<uint32_t>(static_cast<float>(branchWidth) * dir.x));
			branchHeight = glm::max(1u, static_cast<uint32_t>(static_cast<float>(branchHeight) * dir.y));

			uint32_t branchHalfWidth  = static_cast<uint32_t>(glm::round(static_cast<float>(branchWidth) / 2.0f));
			uint32_t branchHalfHeight = static_cast<uint32_t>(glm::round(static_cast<float>(branchHeight) / 2.0f));

			for (int branchX = 0; branchX < branchWidth; ++branchX)
			{
				for (int branchY = 0; branchY < branchHeight; ++branchY)
				{
					size_t absBranchX = (static_cast<size_t>(position.x) - branchHalfWidth) + branchX;
					size_t absBranchY = (static_cast<size_t>(position.y) - branchHalfHeight) + branchY;

					size_t branchIndex = (absBranchY) * pixelGrid.Width + (absBranchX);

					if (branchIndex < world.size()) { world[branchIndex] = GET_PIXEL(PixelType::Wood); }
				}
			}

			if (branchProgress > 0.25f && branchingCounter <= 0 && glm::linearRand(0.0f, 1.0f) > 0.90f && numBranchIterations > 0)
			{
				TreeGenerationSettings branchGenerationSettings = treeGenerationSettings;
				branchGenerationSettings.Length                 = static_cast<size_t>(static_cast<float>(treeGenerationSettings.Length) * (1.05 - branchProgress));
				branchGenerationSettings.StartingThickness      = static_cast<float>(glm::max(branchWidth, branchHeight));
				branchGenerationSettings.StartingAngle          = angle + (glm::linearRand(treeGenerationSettings.AngleRange-20, treeGenerationSettings.AngleRange+20) * branchingDirection);
				branchingCounter                                = branchGenerationSettings.StartingThickness;
				branchingDirection                              = branchingDirection == -1 ? 1 : -1;

				glm::vec2 newMaxPosition = GenerateBranch(position.x, position.y, branchGenerationSettings, numBranchIterations - 1, generateLeafs, noise, world, pixelGrid);
				maxPosition              = glm::vec2(glm::max(maxPosition.x, newMaxPosition.x), glm::max(maxPosition.y, newMaxPosition.y));
			}

			angle += glm::linearRand(-treeGenerationSettings.AngleIncrement, treeGenerationSettings.AngleIncrement);
			angle = glm::clamp(angle,
			                   treeGenerationSettings.StartingAngle - treeGenerationSettings.AngleRange,
			                   treeGenerationSettings.StartingAngle + treeGenerationSettings.AngleRange);

			position += Math::VecFromAngle(angle);

			branchingCounter--;
		}

		if (generateLeafs)
		{
			int64_t leafRadius = static_cast<int64_t>(treeGenerationSettings.Length * 0.5f);
			// Leafs
			for (int64_t leafsX = -leafRadius; leafsX <= leafRadius; ++leafsX)
			{
				for (int64_t leafsY = -leafRadius; leafsY <= leafRadius; ++leafsY)
				{
					int64_t newX = leafsX + position.x;
					int64_t newY = leafsY + position.y;

					maxPosition = glm::vec2(glm::max(maxPosition.x, static_cast<float>(newX)), glm::max(maxPosition.y, static_cast<float>(newY)));

					// Bounds checking
					size_t index = newY * pixelGrid.Width + newX;
					if (index < world.size())
					{
						float randomRadius = leafRadius + (noise[index] * 5.0f);
						//LOG("radius {0}", randomRadius);
						if (world[index].PixelID == PixelType::Air && (leafsX * leafsX + leafsY * leafsY < randomRadius * randomRadius))
						{
							world[index] = GET_PIXEL(PixelType::Leaf);
						}
					}
				}
			}
		}

		return maxPosition;
	}
}
