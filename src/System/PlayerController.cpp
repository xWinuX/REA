#include "REA/System/PlayerController.hpp"

#include <REA/Constants.hpp>
#include <REA/PixelType.hpp>
#include <REA/Component/PixelGrid.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Debug/Performance.hpp>

#include "REA/Assets.hpp"
#include "REA/Component/SpriteRenderer.hpp"

#include "imgui.h"

namespace REA::System
{
	bool checkCollisionForCircle(const glm::ivec2 center, float radius, Component::PixelGrid& pixelGrid)
	{
		for (int y = -radius; y <= radius; ++y)
		{
			for (int x = -radius; x <= radius; ++x)
			{
				if (x * x + y * y <= radius * radius) // Only check within the circle
				{
					glm::ivec2    checkPos = (center + glm::ivec2(x, y)) - glm::ivec2(pixelGrid.ChunkOffset.x  * Constants::CHUNK_SIZE, pixelGrid.ChunkOffset.y * Constants::CHUNK_SIZE);
					Pixel::State* currentPixel;


					if (pixelGrid.TryGetPixelAtPosition(checkPos.x, checkPos.y, currentPixel))
					{
						if (currentPixel->Flags.Has(Pixel::Flags::Solid))
						{
							return true; // Collision detected
						}
					}
					else { return true; }
				}
			}
		}
		return false; // No collision
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


		BENCHMARK_BEGIN
		// Brute force ahh movement, inefficent asf but works
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Transform& transform = transformComponents[i];
			Component::Player&    player    = playerComponents[i];

			if (Input::GetPressed(KeyCode::F)) { player.Position = { 20.0f, 20.0f }; }

			glm::vec2 speed = glm::vec2(Input::GetAxisActionDown(InputAction::Move) * 100.0f * deltaTime, 0.0f) + player.Velocity;

			Component::PixelGrid& pixelGrid = ecs.GetComponent<Component::PixelGrid>(player.PixelGridEntityID);

			glm::ivec2 playerPosition = glm::ivec2(glm::floor(player.Position));
			glm::ivec2 roundedSpeed   = glm::ivec2(glm::floor(speed));
			glm::ivec2 targetPosition = glm::ivec2(glm::floor(player.Position + speed));

			Pixel::State* currentPixel;
			if (checkCollisionForCircle({ targetPosition.x, playerPosition.y }, player.ColliderRadius, pixelGrid))
			{
				// Slope handling logic: Try to move up or down if there's a solid block in front
				int  maxSlopeHeight = 5; // Maximum height the player can "step" onto
				bool foundSlope     = false;

				for (int yOffset = 1; yOffset <= maxSlopeHeight; ++yOffset)
				{
					if (!checkCollisionForCircle({ targetPosition.x, playerPosition.y + yOffset }, player.ColliderRadius, pixelGrid))
					{
						player.Position.y += yOffset; // Step up onto the slope
						foundSlope = true;
						break;
					}
				}

				if (!foundSlope)
				{
					// Check for stepping down
					for (int yOffset = -1; yOffset >= -maxSlopeHeight; --yOffset)
					{
						if (!checkCollisionForCircle({ targetPosition.x, playerPosition.y + yOffset }, player.ColliderRadius, pixelGrid))
						{
							player.Position.y += yOffset; // Step down onto the slope
							foundSlope = true;
							break;
						}
					}
				}

				if (!foundSlope)
				{
					uint32_t checks = 0;
					while (!checkCollisionForCircle({ static_cast<int32_t>(glm::floor(player.Position.x + glm::sign(speed.x))), playerPosition.y }, player.ColliderRadius, pixelGrid))
					{
						player.Position.x += glm::sign(speed.x);
						checks++;
					}

					speed.x           = 0;
					player.Velocity.x = 0;
				}
				else { player.Position.x += speed.x; }
			}
			else { player.Position.x += speed.x; }

			targetPosition = glm::ivec2(glm::floor(player.Position + speed));
			playerPosition = glm::ivec2(glm::floor(player.Position));

			if (checkCollisionForCircle({ playerPosition.x, targetPosition.y }, player.ColliderRadius, pixelGrid))
			{
				uint32_t checks = 0;
				while (!checkCollisionForCircle({ playerPosition.x, static_cast<int32_t>(glm::floor(player.Position.y + glm::sign(speed.y))) }, player.ColliderRadius, pixelGrid) && checks
				       < 10)
				{
					player.Position.y += glm::sign(speed.y);
					checks++;
				}

				speed.y           = 0;
				player.Velocity.y = 0;

				//
			}
			else { player.Position.y += speed.y; }

			glm::ivec2    floorCheck = glm::ivec2(glm::floor(player.Position)) + glm::ivec2(0, -1);
			Pixel::State* floorPixel;
			if (!checkCollisionForCircle({ floorCheck.x, floorCheck.y }, player.ColliderRadius, pixelGrid)) { player.Velocity.y += -2.0f * deltaTime; }
			else if (player.Velocity.y <= 0.0f)
			{
				player.Velocity.y = 0;
				player.CanJump    = true;
			}

			if (player.CanJump && Input::GetButtonActionPressed(InputAction::Jump))
			{
				LOG("Jump");
				player.Velocity.y = 0.6f;
				player.CanJump    = false;
			}

			playerPosition = glm::ivec2(glm::floor(player.Position))- glm::ivec2(pixelGrid.ChunkOffset.x  * Constants::CHUNK_SIZE, pixelGrid.ChunkOffset.y * Constants::CHUNK_SIZE);

			if (pixelGrid.TryGetPixelAtPosition(playerPosition.x, playerPosition.y, currentPixel)) { currentPixel->PixelID = PixelType::Lava; }

			transform.Position = glm::vec3(glm::vec2(player.Position) * 0.1f, 0.0f);
		}
		BENCHMARK_END("Player collisions")
	}
}
