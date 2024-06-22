#pragma once
#include <ranges>
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/Collider.hpp"
#include "REA/Component/Transform.hpp"

#include "box2d/b2_world.h"
#include "REA/Stage.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class Physics final : public ECS::System<Component::Transform, Component::Collider>
	{
		public:
			explicit Physics(bool enableDebugDraw);

			b2World& GetWorld();

			void RegisterDebugDraw(b2Draw* debugDraw);

		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;

			void Execute(Component::Transform*, Component::Collider*, std::vector<uint64_t>& entities, ECS::ContextProvider& context, uint8_t stage) override;

		private:
			b2World _world;
			float   _timeStep           = 1.0f / 60.0f;
			int32   _velocityIterations = 8;
			int32   _positionIterations = 3;
			float   _frameAccumulator   = 0.0f;
			bool    _enableDebugDraw    = false;

			std::vector<uint8_t> _stagesToRun = { Stage::Physics };
	};
}
