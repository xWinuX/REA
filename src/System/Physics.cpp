#include "REA/System/Physics.hpp"

#include <execution>

#include <SplitEngine/Contexts.hpp>

#include <glm/common.hpp>

#include <box2d/b2_fixture.h>
#include <box2d/b2_settings.h>
#include <glm/trigonometric.hpp>
#include <SplitEngine/Application.hpp>

namespace REA::System
{
	Physics::Physics(bool enableDebugDraw):
		_world(b2Vec2(0.0f, -3.0f)),
		_enableDebugDraw(enableDebugDraw) {}

	void Physics::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		_frameAccumulator += contextProvider.GetContext<TimeContext>()->DeltaTime;

		ECS::Registry& registry = contextProvider.GetContext<EngineContext>()->Application->GetECSRegistry();

		while (_frameAccumulator > _timeStep)
		{
			_world.Step(_timeStep, _velocityIterations, _positionIterations);
			registry.ExecuteSystems(false, ECS::Registry::ListBehaviour::Inclusion, _stagesToRun);
			if (_enableDebugDraw) { _world.DebugDraw(); }
			_frameAccumulator -= _timeStep;
		}

		System<Component::Transform, Component::Collider>::ExecuteArchetypes(archetypes, contextProvider, stage);
	}

	void Physics::Execute(Component::Transform*  transformComponents,
	                      Component::Collider*   colliderComponents,
	                      std::vector<uint64_t>& entities,
	                      ECS::ContextProvider&  context,
	                      uint8_t                stage)
	{
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Collider&  collider  = colliderComponents[i];
			Component::Transform& transform = transformComponents[i];

			// Create body if needed
			if (collider.Body == nullptr)
			{
				collider.World = &_world;

				b2BodyDef bodyDef = collider.PhysicsMaterial->GetBodyDefCopy();
				bodyDef.type      = collider.InitialType;
				bodyDef.position  = { transform.Position.x, transform.Position.y };
				bodyDef.angle     = glm::radians(transform.Rotation);
				collider.Body     = _world.CreateBody(&bodyDef);


				b2FixtureDef fixtureDef = collider.PhysicsMaterial->GetFixtureDefCopy();
				for (b2PolygonShape& initialShape: collider.InitialShapes)
				{
					fixtureDef.shape   = &initialShape;

					for (int j = 0; j < initialShape.m_count; ++j)
					{
						LOG("vertex 1 x: {0} y: {1}", initialShape.m_vertices[j].x, initialShape.m_vertices[j].y);

					}

					b2Fixture* fixture = collider.Body->CreateFixture(&fixtureDef);
					collider.Fixtures.push_back(fixture);
				}

				// TODO: Maybe free memory completely (only here so i don't forget that i can optimize here if needed)
				collider.InitialShapes.clear();

				collider.Body->GetUserData().EntityID = entities[i];
			}

			if (collider.Body->GetType() != b2BodyType::b2_staticBody)
			{
				const b2Vec2& position = collider.Body->GetPosition();

				transform.Position.x = position.x;
				transform.Position.y = position.y;

				transform.Rotation = glm::degrees(-collider.Body->GetAngle());
			}
		}
	}

	b2World& Physics::GetWorld() { return _world; }
	void     Physics::RegisterDebugDraw(b2Draw* debugDraw) { _world.SetDebugDraw(debugDraw); }
}
