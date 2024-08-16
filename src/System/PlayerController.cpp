#include "REA/System/PlayerController.hpp"

#include <REA/Constants.hpp>
#include <REA/PixelType.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Debug/Performance.hpp>

#include "REA/Assets.hpp"
#include "REA/Component/SpriteRenderer.hpp"

#include "imgui.h"

namespace REA::System
{
	bool PlayerController::CheckCollisionForCircle(glm::vec2 center, int radius, Component::PixelGrid& pixelGrid)
	{
		glm::ivec2 flooredCenter = glm::ivec2(glm::floor(center));

		for (int y = -radius; y <= radius; ++y)
		{
			for (int x = -radius; x <= radius; ++x)
			{
				if (x * x + y * y <= radius * radius)
				{
					glm::ivec2 checkPos = (flooredCenter + glm::ivec2(x, y)) - glm::ivec2(pixelGrid.ChunkOffset.x * Constants::CHUNK_SIZE,
					                                                                      pixelGrid.ChunkOffset.y * Constants::CHUNK_SIZE);
					Pixel::State* currentPixel;

					if (pixelGrid.TryGetPixelAtPosition(checkPos.x, checkPos.y, currentPixel)) { if (currentPixel->Flags.Has(Pixel::Flags::Solid)) { return true; } }
					else { return true; }
				}
			}
		}
		return false;
	}

	float PlayerController::GetAverageDensity(glm::vec2 center, int radius, Component::PixelGrid& pixelGrid)
	{
		float      density       = 0.0f;
		uint32_t   numPixels     = 0;
		glm::ivec2 flooredCenter = glm::ivec2(glm::floor(center));
		for (int y = -radius; y <= radius; ++y)
		{
			for (int x = -radius; x <= radius; ++x)
			{
				if (x * x + y * y <= radius * radius)
				{
					glm::ivec2 checkPos = (flooredCenter + glm::ivec2(x, y)) - glm::ivec2(pixelGrid.ChunkOffset.x * Constants::CHUNK_SIZE,
					                                                                      pixelGrid.ChunkOffset.y * Constants::CHUNK_SIZE);
					Pixel::State* currentPixel;

					if (pixelGrid.TryGetPixelAtPosition(checkPos.x, checkPos.y, currentPixel))
					{
						if (!currentPixel->Flags.Has(Pixel::Flags::Solid))
						{
							density += static_cast<float>(pixelGrid.PixelDataLookup[currentPixel->PixelID].Density);
							numPixels++;
						}
					}
				}
			}
		}

		return density / static_cast<float>(numPixels);
	}

	void PlayerController::Execute(Component::Transform*  transformComponents,
	                               Component::Player*     playerComponents,
	                               Component::Collider*   colliderComponents,
	                               std::vector<uint64_t>& entities,
	                               ECS::ContextProvider&  contextProvider,
	                               uint8_t                stage)
	{
		ECS::Registry& ecs       = contextProvider.GetContext<EngineContext>()->Application->GetECSRegistry();
		float          deltaTime = contextProvider.GetContext<TimeContext>()->DeltaTime;


		//BENCHMARK_BEGIN
		// Brute force ahh movement, inefficent asf but works
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Transform& transform = transformComponents[i];
			Component::Player&    player    = playerComponents[i];


			glm::vec2 playerPosition = glm::vec2(transform.Position);

			if (player.PixelGridEntityID == -1ull) { continue; }

			Component::PixelGrid& pixelGrid = ecs.GetComponent<Component::PixelGrid>(player.PixelGridEntityID);

			if (!player.NoClip)
			{
				float averageDensity = GetAverageDensity(playerPosition, player.ColliderRadius, pixelGrid);


				float xFriction = 1.0f / (1.0f + (deltaTime * (averageDensity)));
				float yFriction = 1.0f / (1.0f + (deltaTime * (glm::clamp(averageDensity - static_cast<float>(pixelGrid.PixelDataLookup[PixelType::Air].Density),
				                                                          0.0f,
				                                                          std::numeric_limits<float>::max()))));

				float jumpStrength = player.JumpStrength / (1.0f + 0.1f * sqrt(averageDensity));

				bool grounded = CheckCollisionForCircle({ playerPosition.x, playerPosition.y - 1 }, player.ColliderRadius, pixelGrid);
				if (!grounded) { player.Velocity.y += player.Gravity * deltaTime; }
				//else if (player.Velocity.y < 0) { player.Velocity.y = 0; }

				player.Velocity.x *= xFriction;
				player.Velocity.y *= yFriction;

				float moveDirection = Input::GetAxisActionDown(InputAction::Move);

				float horizontalSpeed = player.Acceleration * moveDirection * deltaTime;

				if (moveDirection != 0.0f)
				{
					if (glm::abs(player.Velocity.x) + glm::abs(horizontalSpeed) < player.MaxSpeed) { player.Velocity.x += horizontalSpeed; }
					else { player.Velocity.x = player.MaxSpeed * glm::sign(player.Velocity.x); }
				}

				if (grounded && Input::GetButtonActionPressed(InputAction::Jump)) { player.Velocity.y = jumpStrength; }

				if (CheckCollisionForCircle({ playerPosition.x + (player.Velocity.x * deltaTime), playerPosition.y }, player.ColliderRadius, pixelGrid))
				{
					// Slope handling logic: Try to move up or down if there's a solid block in front
					int  maxSlopeHeight = 5; // Maximum height the player can "step" onto
					bool foundSlope     = false;

					for (int mult = -1; mult < 2; mult += 2)
					{
						for (int yOffset = 1; yOffset <= maxSlopeHeight; ++yOffset)
						{
							float offset = static_cast<float>(yOffset * mult);
							if (!CheckCollisionForCircle({ playerPosition.x + (player.Velocity.x * deltaTime), playerPosition.y + offset }, player.ColliderRadius, pixelGrid))
							{
								playerPosition.y += offset; // Step up onto the slope
								foundSlope = true;
								break;
							}
						}

						if (foundSlope) { break; }
					}

					if (!foundSlope)
					{
						uint32_t checks = 0;
						float    offset = glm::sign(player.Velocity.x * deltaTime);
						while (!CheckCollisionForCircle({ playerPosition.x + offset, playerPosition.y }, player.ColliderRadius, pixelGrid) && checks < 10)
						{
							playerPosition.x += offset;
							checks++;
						}

						player.Velocity.x = 0;
					}
				}

				if (CheckCollisionForCircle({ playerPosition.x, playerPosition.y + (player.Velocity.y * deltaTime) }, player.ColliderRadius, pixelGrid))
				{
					uint32_t checks = 0;
					float    offset = glm::sign(player.Velocity.y * deltaTime);
					while (!CheckCollisionForCircle({ playerPosition.x, playerPosition.y + offset }, player.ColliderRadius, pixelGrid) && checks < 10)
					{
						playerPosition.y += offset;
						checks++;
					}

					player.Velocity.y = 0;
				}
			}
			else { player.Velocity = Input::GetAxis2DActionDown(InputAction::Fly) * 1000.0f; }

			playerPosition += player.Velocity * deltaTime;

			LOG("player postion x {0} y {1}", playerPosition.x, playerPosition.y);

			transform.Position = glm::vec3(playerPosition, 0.0f);
		}
		//		BENCHMARK_END("Player collisions")
	}
}
