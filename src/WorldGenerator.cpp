#include "REA/WorldGenerator.hpp"

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
		int seed = 1234;

		auto caveNoiseGenerator = FastNoise::NewFromEncodedNodeTree("IgAAAAA/j8J1Pg0AAwAAAB+FE0EQAM3MDEATAJqZGT8LAAAAAAAAAAAAAQAAAAQAAAAA9iicPwAfhes+AArXoz0ArkeNQQ==");
		std::vector<float> caveNoise = std::vector<float>(world.size());
		caveNoiseGenerator->GenUniformGrid2D(caveNoise.data(), 0, 0, pixelGrid.WorldWidth, pixelGrid.WorldHeight, 0.01f * generationSettings.CaveNoiseFrequency, seed);

		auto terrainNoiseGenerator = FastNoise::NewFromEncodedNodeTree("IQATAClcDz4UACkAAAAAgD8AAACAPwAAAAAAAAAAAAAQAOxR+D8ZAA0AAwAAAD0KF0ApAAAAAAA/AHsUrj4AzczMPQCPwnU+AAAAAD8=");
		std::vector<float> terrainNoise = std::vector<float>(world.size());
		std::vector<float> caveTerrainTransitionNoise = std::vector<float>(world.size());
		terrainNoiseGenerator->GenUniformGrid2D(terrainNoise.data(), 0, 0, pixelGrid.WorldWidth, 1, 0.01f * generationSettings.OverworldNoiseFrequency, seed);
		terrainNoiseGenerator->GenUniformGrid2D(caveTerrainTransitionNoise.data(), 0, 100, pixelGrid.WorldWidth, 1, 0.01f, seed);

		auto normalNoiseGenerator = FastNoise::NewFromEncodedNodeTree("IQATAClcDz4UACkAAAAAgD8AAACAPwAAAAAAAAAAAAAQAOxR+D8ZAA0AAwAAAD0KF0ApAAAAAAA/AHsUrj4AzczMPQCPwnU+AAAAAD8=");
		std::vector<float> normalNoise = std::vector<float>(world.size());
		normalNoiseGenerator->GenUniformGrid2D(normalNoise.data(), 0, 0, pixelGrid.WorldWidth, pixelGrid.WorldHeight, 0.1f, seed);

		float caveBaseLine = (generationSettings.TerrainBaseline - generationSettings.TerrainRange);

		FastNoise::SmartNode<FastNoise::White> whiteNoiseGenerator = FastNoise::New<FastNoise::White>();

		for (int i = 0; i < world.size(); ++i)
		{
			size_t x = i % pixelGrid.WorldWidth;
			size_t y = i / pixelGrid.WorldWidth;

			float caveThreshold = generationSettings.CaveNoiseTreshold;
			if (y > caveBaseLine)
			{
				float a       = glm::clamp((static_cast<float>(y) - caveBaseLine) / (generationSettings.TerrainBaseline - caveBaseLine), 0.0f, 1.0f);
				caveThreshold = glm::mix(generationSettings.CaveNoiseTreshold, generationSettings.CaveNoiseTresholdTerrain, a);
			}

			if (caveNoise[i] < caveThreshold)
			{
				float caveHeight = caveBaseLine + (caveTerrainTransitionNoise[x] * generationSettings.CaveRange);
				if (y < static_cast<size_t>(caveHeight)) { world[i] = GET_PIXEL(PixelType::Stone); }
				else
				{
					float terrainHeight = generationSettings.TerrainBaseline + (terrainNoise[x] * generationSettings.TerrainRange);

					if (y < static_cast<size_t>(terrainHeight)) { world[i] = GET_PIXEL(PixelType::Dirt); }
					else if (y < static_cast<size_t>(terrainHeight) + generationSettings.GrassHeight) { world[i] = GET_PIXEL(PixelType::Grass); }
				}
			}
		}


		// Trees
		/*
		float nextTreeEdge = 0;
		int   treePadding  = 400;
		for (int x = treePadding; x < pixelGrid.WorldWidth - treePadding; ++x)
		{
			if (x > nextTreeEdge + (treePadding))
			{
				for (size_t y = pixelGrid.WorldHeight - 1; y > 0; --y)
				{
					glm::vec2 position   = { x, y };
					size_t    index      = y * pixelGrid.WorldWidth + x;
					size_t    belowIndex = (y - 1) * pixelGrid.WorldWidth + x;
					if (world[belowIndex].PixelID == PixelType::Grass)
					{
						uint32_t treeHeight = 100;
						uint32_t treeWidth  = 10;
						float    xOffset    = 0;

						TreeGenerationSettings treeGenerationSettings{};

						// Tree and leafs
						treeGenerationSettings.Length            = 500;
						treeGenerationSettings.StartingThickness = 50;
						treeGenerationSettings.StartingAngle     = 90.0f;
						treeGenerationSettings.AngleIncrement    = 0.5f;
						treeGenerationSettings.AngleRange        = 45;

						nextTreeEdge = GenerateBranch(position.x, std::max(position.y - 10.0f, 0.0f), treeGenerationSettings, 5, 0, true, whiteNoiseGenerator, seed, world, pixelGrid).x;

						// Roots
						treeGenerationSettings.Length            = 200;
						treeGenerationSettings.StartingThickness = 50;
						treeGenerationSettings.AngleIncrement    = 5.0f;
						treeGenerationSettings.AngleRange        = 45;
						treeGenerationSettings.StartingAngle     = -90;

						GenerateBranch(position.x, std::max(position.y - 10.0f, 0.0f), treeGenerationSettings, 5, 0, false, whiteNoiseGenerator, seed, world, pixelGrid);

						treeGenerationSettings.StartingThickness = 30;
						treeGenerationSettings.StartingAngle     = -90 - 45;
						GenerateBranch(position.x, std::max(position.y - 10.0f, 0.0f), treeGenerationSettings, 4, 0, false, whiteNoiseGenerator, seed, world, pixelGrid);

						treeGenerationSettings.StartingAngle = -90 + 45;
						GenerateBranch(position.x, std::max(position.y - 10.0f, 0.0f), treeGenerationSettings, 4, 0, false, whiteNoiseGenerator, seed, world, pixelGrid);

						break;
					}
				}
			}
		}*/
	}

	glm::vec2 WorldGenerator::GenerateBranch(float                                   startX,
	                                         float                                   startY,
	                                         TreeGenerationSettings&                 treeGenerationSettings,
	                                         size_t                                  numBranchIterations,
	                                         size_t                                  currentIteration,
	                                         bool                                    generateLeafs,
	                                         FastNoise::SmartNode<FastNoise::White>& noiseGenerator,
	                                         int                                     seed,
	                                         std::vector<Pixel::State>&              world,
	                                         Component::PixelGrid&                   pixelGrid)
	{
		// Branches
		glm::vec2 maxPosition = { startX, startY };

		glm::vec2 position{};
		float     angle{};

		for (int i = 0; i < 2; ++i)
		{
			position = { startX, startY };
			angle    = treeGenerationSettings.StartingAngle;

			int32_t branchingThicknessSpacingCounter = 0;
			float   branchingDirection               = noiseGenerator->GenSingle2D(position.x, position.y, 1234) < 0.0f ? -1.0f : 1.0f;

			for (int branchStep = 0; branchStep < treeGenerationSettings.Length; ++branchStep)
			{
				float branchProgress = static_cast<float>(branchStep) / static_cast<float>(treeGenerationSettings.Length);

				glm::vec2 dir = glm::abs(Math::VecFromAngle(angle + 90));

				uint32_t adjustedThickness = static_cast<uint32_t>(glm::round(static_cast<float>(treeGenerationSettings.StartingThickness) * (1.0f - branchProgress)));
				uint32_t thickness         = glm::max(treeGenerationSettings.MinThickness, adjustedThickness);

				uint32_t branchWidth  = thickness;
				uint32_t branchHeight = branchWidth;

				branchWidth  = glm::max(1u, static_cast<uint32_t>(static_cast<float>(branchWidth) * dir.x));
				branchHeight = glm::max(1u, static_cast<uint32_t>(static_cast<float>(branchHeight) * dir.y));

				uint32_t branchHalfWidth  = static_cast<uint32_t>(glm::round(static_cast<float>(branchWidth) / 2.0f));
				uint32_t branchHalfHeight = static_cast<uint32_t>(glm::round(static_cast<float>(branchHeight) / 2.0f));

				if (i == 0)
				{
					for (int branchX = 0; branchX < branchWidth; ++branchX)
					{
						for (int branchY = 0; branchY < branchHeight; ++branchY)
						{
							size_t absBranchX = (static_cast<size_t>(position.x) - branchHalfWidth) + branchX;
							size_t absBranchY = (static_cast<size_t>(position.y) - branchHalfHeight) + branchY;

							size_t branchIndex = (absBranchY) * pixelGrid.WorldWidth + (absBranchX);

							if (branchIndex < world.size()) { world[branchIndex] = GET_PIXEL(PixelType::Wood); }
						}
					}
				}
				else
				{
					if (branchProgress > treeGenerationSettings.BranchingStart && branchingThicknessSpacingCounter <= 0 &&
					    glm::abs(noiseGenerator->GenSingle2D(position.x, position.y, seed + 1)) > (1.0f - treeGenerationSettings.BranchingChance) && numBranchIterations > 0)
					{
						TreeGenerationSettings branchGenerationSettings = treeGenerationSettings;

						float lengthAdd = glm::abs(noiseGenerator->GenSingle2D(position.x, position.y, seed + 2)) * 0.6f;

						branchGenerationSettings.Length = static_cast<uint32_t>(static_cast<float>(treeGenerationSettings.Length) * (1.0f - branchProgress) * (0.5f + lengthAdd));
						branchGenerationSettings.StartingThickness = glm::max(branchWidth, branchHeight);

						float angleAdd                         = noiseGenerator->GenSingle2D(position.x, position.y, seed + 3) * 20;
						branchGenerationSettings.StartingAngle = angle + ((treeGenerationSettings.AngleRange + angleAdd) * branchingDirection);

						// Check for other branches and if found reduce length
						float     rayCastAngle    = branchGenerationSettings.StartingAngle;
						glm::vec2 rayCastPosition = position;
						for (int i = 0; i < branchGenerationSettings.Length; ++i)
						{
							size_t index = static_cast<size_t>(rayCastPosition.y) * pixelGrid.WorldWidth + static_cast<size_t>(rayCastPosition.x);
							if (i > branchGenerationSettings.StartingThickness + 4 && index < world.size() && world[index].PixelID == PixelType::Wood)
							{
								branchGenerationSettings.Length = glm::max(1, i - 10);
								break;
							}

							rayCastAngle += noiseGenerator->GenSingle2D(rayCastPosition.x, rayCastPosition.y, seed) * branchGenerationSettings.AngleIncrement;
							rayCastAngle = glm::clamp(rayCastAngle,
							                          branchGenerationSettings.StartingAngle - branchGenerationSettings.AngleRange,
							                          branchGenerationSettings.StartingAngle + branchGenerationSettings.AngleRange);

							rayCastPosition += Math::VecFromAngle(rayCastAngle);
						}

						branchingThicknessSpacingCounter = static_cast<int32_t>(branchGenerationSettings.StartingThickness);
						branchingDirection               = branchingDirection == -1 ? 1 : -1;

						glm::vec2 newMaxPosition = GenerateBranch(position.x,
						                                          position.y,
						                                          branchGenerationSettings,
						                                          numBranchIterations - 1,
						                                          currentIteration + 1,
						                                          generateLeafs,
						                                          noiseGenerator,
						                                          seed,
						                                          world,
						                                          pixelGrid);

						maxPosition = glm::vec2(glm::max(maxPosition.x, newMaxPosition.x), glm::max(maxPosition.y, newMaxPosition.y));
					}

					branchingThicknessSpacingCounter--;
				}

				angle += noiseGenerator->GenSingle2D(position.x, position.y, seed) * treeGenerationSettings.AngleIncrement;
				angle = glm::clamp(angle,
				                   treeGenerationSettings.StartingAngle - treeGenerationSettings.AngleRange,
				                   treeGenerationSettings.StartingAngle + treeGenerationSettings.AngleRange);

				position += Math::VecFromAngle(angle);
			}
		}


		if (generateLeafs)
		{
			int64_t leafRadius = treeGenerationSettings.LeadRadius;
			// Leafs
			for (int64_t leafsX = -leafRadius; leafsX <= leafRadius; ++leafsX)
			{
				for (int64_t leafsY = -leafRadius; leafsY <= leafRadius; ++leafsY)
				{
					int64_t newX = leafsX + static_cast<int64_t>(position.x);
					int64_t newY = leafsY + static_cast<int64_t>(position.y);

					maxPosition = glm::vec2(glm::max(maxPosition.x, static_cast<float>(newX)), glm::max(maxPosition.y, static_cast<float>(newY)));

					// Bounds checking
					size_t index = newY * pixelGrid.WorldWidth + newX;
					if (index < world.size())
					{
						float randomRadius = static_cast<float>(leafRadius) + (noiseGenerator->GenSingle2D(static_cast<float>(newX), static_cast<float>(newY), seed) * 1.0f);
						if (world[index].PixelID == PixelType::Air && (static_cast<float>(leafsX * leafsX + leafsY * leafsY) < randomRadius * randomRadius))
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
