#pragma once
#include <SplitEngine/AssetDatabase.hpp>
#include <SplitEngine/ECS/System.hpp>

#include "REA/SpriteTexture.hpp"
#include "REA/Component/Physics.hpp"
#include "REA/Component/Player.hpp"
#include "REA/Component/Transform.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class PlayerController final : public ECS::System<Component::Transform, Component::Player, Component::Physics>
	{
		public:
			PlayerController(AssetHandle<SpriteTexture> bulletSprite);

			void Execute(Component::Transform*  transformComponents,
			             Component::Player*     playerComponents,
			             Component::Physics*    physicsComponents,
			             std::vector<uint64_t>& entities,
			             ECS::Context&          context) override;

		private:
			AssetHandle<SpriteTexture> _bulletSprite;
	};
}
