#pragma once
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>

namespace REA
{
	class PhysicsMaterial
	{
		public:
			struct CreateInfo
			{
				float LinearDamping        = 0.0f;
				float AngularDamping       = 0.0f;
				float GravityScale         = 1.0f;
				float Friction             = 0.2f;
				float Restitution          = 0.0f;
				float RestitutionThreshold = 1.0f * b2_lengthUnitsPerMeter;
				float Density              = 0.0f;
			};

		public:
			explicit PhysicsMaterial(const CreateInfo& createInfo);

			[[nodiscard]] const b2BodyDef& GetBodyDef() const;
			[[nodiscard]] b2BodyDef        GetBodyDefCopy() const;

			[[nodiscard]] const b2FixtureDef& GetFixtureDef() const;
			[[nodiscard]] b2FixtureDef        GetFixtureDefCopy() const;

		private:
			b2BodyDef    _bodyDef{};
			b2FixtureDef _fixtureDef{};
	};
}
