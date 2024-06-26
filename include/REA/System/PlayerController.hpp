#pragma once
#include <SplitEngine/AssetDatabase.hpp>
#include <SplitEngine/ECS/System.hpp>

#include "REA/SpriteTexture.hpp"
#include "REA/Component/Collider.hpp"
#include "REA/Component/Physics.hpp"
#include "REA/Component/Player.hpp"
#include "REA/Component/Transform.hpp"


using namespace SplitEngine;

namespace REA::System
{
	class PlayerController final : public ECS::System<Component::Transform, Component::Player, Component::Collider>
	{
		protected:
			void Execute(Component::Transform*  transformComponents,
			             Component::Player*     playerComponents,
			             Component::Collider*   colliderComponents,
			             std::vector<uint64_t>& entities,
			             ECS::ContextProvider&  contextProvider,
			             uint8_t                stage) override;
	};
}
