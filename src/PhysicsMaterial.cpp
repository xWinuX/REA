#include "../include/REA/PhysicsMaterial.hpp"

namespace REA
{
	PhysicsMaterial::PhysicsMaterial(const CreateInfo& createInfo)
	{
		_bodyDef.linearDamping  = createInfo.LinearDamping;
		_bodyDef.angularDamping = createInfo.AngularDamping;
		_bodyDef.gravityScale   = createInfo.GravityScale;

		_fixtureDef.friction             = createInfo.Friction;
		_fixtureDef.restitution          = createInfo.Restitution;
		_fixtureDef.restitutionThreshold = createInfo.RestitutionThreshold;
		_fixtureDef.density              = createInfo.Density;
	}

	const b2BodyDef& PhysicsMaterial::GetBodyDef() const { return _bodyDef; }

	b2BodyDef PhysicsMaterial::GetBodyDefCopy() const { return _bodyDef; }
	
	const b2FixtureDef& PhysicsMaterial::GetFixtureDef() const { return _fixtureDef; }

	b2FixtureDef PhysicsMaterial::GetFixtureDefCopy() const { return _fixtureDef; }
}
