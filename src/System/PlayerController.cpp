#include "REA/System/PlayerController.hpp"

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
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Transform& transformComponent = transformComponents[i];
			Component::Player&    playerComponent    = playerComponents[i];
			Component::Collider&  collider           = colliderComponents[i];

			glm::vec2 inputAxis = Input::GetAxis2DActionDown(InputAction::Move);


			if (collider.Body != nullptr)
			{
				b2Vec2 force = { inputAxis.x, inputAxis.y };
				force *= 10.0f;
				b2Vec2 point = { 0, 0 };
				collider.Body->SetLinearVelocity(force);
			}
		}
	}
}
