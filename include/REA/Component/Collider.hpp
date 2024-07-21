#pragma once

#include <SplitEngine/AssetDatabase.hpp>

#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_body.h>
#include <box2d/b2_world.h>
#include <glm/vec3.hpp>

#include "REA/PhysicsMaterial.hpp"

using namespace SplitEngine;

namespace REA::Component
{
	struct Collider
	{
		~Collider() { if (World != nullptr && Body != nullptr) { World->DestroyBody(Body); } }

		AssetHandle<PhysicsMaterial> PhysicsMaterial;
		b2BodyType                   InitialType = b2BodyType::b2_staticBody;
		std::vector<b2PolygonShape>  InitialShapes = std::vector<b2PolygonShape>(0);
		std::vector<b2Fixture*>      Fixtures = std::vector<b2Fixture*>(0);

		b2World* World = nullptr;
		b2Body*  Body  = nullptr;

		glm::vec3 PreviousPosition = {0.0f , 0.0f, 0.0f};
	};
}
