#pragma once

#include <SplitEngine/AssetDatabase.hpp>

#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_body.h>
#include <box2d/b2_world.h>

#include "REA/PhysicsMaterial.hpp"

using namespace SplitEngine;

namespace REA::Component
{
	struct Collider
	{
		~Collider() { if (World != nullptr && Body != nullptr) { World->DestroyBody(Body); } }

		AssetHandle<PhysicsMaterial> PhysicsMaterial;
		b2BodyType                   InitialType = b2BodyType::b2_staticBody;
		std::vector<b2PolygonShape>  InitialShapes {};
		std::vector<b2Fixture*>      Fixtures {};

		b2World* World = nullptr;
		b2Body*  Body  = nullptr;
	};
}
