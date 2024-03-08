#include "REA/System/PlayerController.hpp"

#include <SplitEngine/Application.hpp>
#include <SplitEngine/Input.hpp>

#include "REA/Assets.hpp"
#include "REA/Component/SpriteRenderer.hpp"

#include "imgui.h"

namespace REA::System
{
	PlayerController::PlayerController(const AssetHandle<SpriteTexture> bulletSprite):
		_bulletSprite(bulletSprite) {}

	void PlayerController::Execute(Component::Transform*  transformComponents,
	                               Component::Player*     playerComponents,
	                               Component::Physics*    physicsComponents,
	                               std::vector<uint64_t>& entities,
	                               ECS::Context&          context)
	{
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Transform& transformComponent = transformComponents[i];
			Component::Player&    playerComponent    = playerComponents[i];
			Component::Physics&   physicsComponent   = physicsComponents[i];

			glm::vec2 inputAxis = Input::GetAxis2DActionDown(InputAction::Move);

			glm::vec2 direction = glm::normalize(glm::vec3(Input::GetMousePosition(), 0.0f) - transformComponent.Position);

			transformComponent.Rotation = glm::degrees(glm::atan(direction.y, direction.x));
			physicsComponent.Velocity   = glm::vec3(inputAxis * 10.0f, 0.0f);

			if (Input::GetButtonActionDown(InputAction::Fire))
			{
				context.Registry->CreateEntity<Component::Transform, Component::Physics, Component::SpriteRenderer>({ transformComponent.Position, transformComponent.Rotation },
				                                                                                                    { false, glm::vec3(direction.x, -direction.y, 0.0f) * 3.0f },
				                                                                                                    { _bulletSprite, 5.0f });
			}
		}
	}
}
