#include "REA/System/PlayerController.hpp"

#include <REA/Constants.hpp>
#include <REA/PixelType.hpp>
#include <REA/Component/PixelGrid.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>

#include "REA/Assets.hpp"
#include "REA/Component/SpriteRenderer.hpp"

#include "imgui.h"

namespace REA::System
{
	void PlayerController::Execute(Component::Transform*  transformComponents,
	                               Component::Player*     playerComponents,
	                               Component::Collider*   colliderComponents,
	                               std::vector<uint64_t>& entities,
	                               ECS::ContextProvider&  contextProvider,
	                               uint8_t                stage)
	{
		ECS::Registry& ecs       = contextProvider.GetContext<EngineContext>()->Application->GetECSRegistry();
		float          deltaTime = contextProvider.GetContext<TimeContext>()->DeltaTime;


		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Transform& transform = transformComponents[i];
			Component::Player&    player    = playerComponents[i];

			glm::vec2 speed = glm::vec2(Input::GetAxisActionDown(InputAction::Move) * 100.0f * deltaTime, 0.0f) + player.Velocity;

			Component::PixelGrid& pixelGrid = ecs.GetComponent<Component::PixelGrid>(player.PixelGridEntityID);

			glm::ivec2 playerPosition = glm::ivec2(glm::floor(player.Position));
			glm::ivec2 roundedSpeed   = glm::ivec2(glm::floor(speed));
			glm::ivec2 targetPosition = glm::ivec2(glm::floor(player.Position + speed));

			LOG("player position x {0} y {1}", player.Position.x, player.Position.y);

			Pixel::State* currentPixel;
			if (pixelGrid.TryGetPixelAtPosition(targetPosition.x, playerPosition.y, currentPixel))
			{
				if (currentPixel->Flags.Has(Pixel::Flags::Solid))
				{
					// Slope handling logic: Try to move up or down if there's a solid block in front
					int  maxSlopeHeight = 5; // Maximum height the player can "step" onto
					bool foundSlope     = false;

					for (int yOffset = 1; yOffset <= maxSlopeHeight; ++yOffset)
					{
						if (pixelGrid.TryGetPixelAtPosition(targetPosition.x, playerPosition.y + yOffset, currentPixel) && !currentPixel->Flags.Has(Pixel::Flags::Solid))
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
							if (pixelGrid.TryGetPixelAtPosition(targetPosition.x, playerPosition.y + yOffset, currentPixel) && !currentPixel->Flags.Has(Pixel::Flags::Solid))
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
						while (pixelGrid.TryGetPixelAtPosition(static_cast<int32_t>(glm::floor(player.Position.x + glm::sign(speed.x))), playerPosition.y, currentPixel) && !
						       currentPixel->Flags.Has(Pixel::Flags::Solid) && checks < 10)
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
			}

			targetPosition = glm::ivec2(glm::floor(player.Position + speed));
			playerPosition = glm::ivec2(glm::floor(player.Position));

			if (pixelGrid.TryGetPixelAtPosition(playerPosition.x, targetPosition.y, currentPixel))
			{
				if (currentPixel->Flags.Has(Pixel::Flags::Solid))
				{
					uint32_t checks = 0;
					while (pixelGrid.TryGetPixelAtPosition(playerPosition.x, static_cast<int32_t>(glm::floor(player.Position.y + glm::sign(speed.y))), currentPixel) && !
					       currentPixel->Flags.Has(Pixel::Flags::Solid) && checks < 10)
					{
						player.Position.y += glm::sign(speed.y);
						checks++;
					}

					speed.y           = 0;
					player.Velocity.y = 0;
				}
				else { player.Position.y += speed.y; }
			}

			glm::ivec2    floorCheck = glm::ivec2(glm::floor(player.Position)) + glm::ivec2(0, -1);
			Pixel::State* floorPixel;
			if (pixelGrid.TryGetPixelAtPosition(floorCheck.x, floorCheck.y, floorPixel) && !floorPixel->Flags.Has(Pixel::Flags::Solid)) { player.Velocity.y += -2.0f * deltaTime; }
			else if (player.Velocity.y <= 0.0f)
			{
				player.Velocity.y = 0;
				player.CanJump    = true;
			}

			if (player.CanJump && Input::GetButtonActionPressed(InputAction::Jump))
			{
				LOG("Jump");
				player.Velocity.y = 0.3f;
				player.CanJump    = false;
			}

			playerPosition = glm::ivec2(glm::floor(player.Position));

			if (pixelGrid.TryGetPixelAtPosition(playerPosition.x, playerPosition.y, currentPixel)) { currentPixel->PixelID = PixelType::Lava; }

			transform.Position = glm::vec3(glm::vec2(player.Position) * 0.1f, 0.0f);
		}
	}
}
