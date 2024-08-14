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
					bool grounded = CheckCollisionForCircle({ playerPosition.x, playerPosition.y - 1 }, player.ColliderRadius, pixelGrid);
					if (!grounded)
					{
						player.Velocity.y += -700.0f * deltaTime;
					}
					else if (player.Velocity.y < 0)
					{
						player.Velocity.y = 0;
					}


					float moveDirection   = Input::GetAxisActionDown(InputAction::Move);
					float acceleration    = 20.0f;
					float maxSpeed        = 200.0f;
					float gravity = 100.0f;
					float horizontalSpeed = acceleration * moveDirection * deltaTime;

					player.Velocity.x = moveDirection * maxSpeed;

					//if (moveDirection != 0.0f)
					//{
					//	if (abs(player.Velocity.x + horizontalSpeed) < maxSpeed) { player.Velocity.x += horizontalSpeed; }
					//	else { player.Velocity.x = maxSpeed; }
					//}

					if (grounded && Input::GetButtonActionPressed(InputAction::Jump)) { player.Velocity.y = 400.0f; }



					if (CheckCollisionForCircle({ playerPosition.x + (player.Velocity.x * deltaTime), playerPosition.y }, player.ColliderRadius, pixelGrid))
					{
						// Slope handling logic: Try to move up or down if there's a solid block in front
						int  maxSlopeHeight = 5; // Maximum height the player can "step" onto
						bool foundSlope     = false;

						for (int yOffset = 1; yOffset <= maxSlopeHeight; ++yOffset)
						{
							float offset = static_cast<float>(yOffset);
							if (!CheckCollisionForCircle({ playerPosition.x + (player.Velocity.x * deltaTime), playerPosition.y + offset }, player.ColliderRadius, pixelGrid))
							{
								playerPosition.y += offset; // Step up onto the slope
								foundSlope = true;
								break;
							}
						}

						if (!foundSlope)
						{
							// Check for stepping down
							for (int yOffset = -1; yOffset >= -maxSlopeHeight; --yOffset)
							{
								float offset = static_cast<float>(yOffset);
								if (!CheckCollisionForCircle({ playerPosition.x + (player.Velocity.x * deltaTime), playerPosition.y + offset }, player.ColliderRadius, pixelGrid))
								{
									playerPosition.y += offset;
									foundSlope = true;
									break;
								}
							}
						}

						if (!foundSlope)
						{
							uint32_t checks = 0;
							float    offset = glm::sign(player.Velocity.x * deltaTime);
							while (!CheckCollisionForCircle({ playerPosition.x + offset, playerPosition.y }, player.ColliderRadius, pixelGrid))
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
				else
				{
					player.Velocity = Input::GetAxis2DActionDown(InputAction::Fly) * 1000.0f;
				}

				playerPosition += player.Velocity * deltaTime;

				LOG("player postion x {0} y {1}", playerPosition.x, playerPosition.y);

				transform.Position = glm::vec3(playerPosition, 0.0f);
			}
//		BENCHMARK_END("Player collisions")
	}
}
