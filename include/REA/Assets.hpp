#pragma once

namespace REA
{
	enum class InputAction
	{
		Move,
		Rotate,
		Fire,
	};

	namespace Asset
	{
		enum class Shader
		{
			Sprite,
			PixelGrid,
			PhysicsDebug,
			Comp_PixelGrid_Idle,
			Comp_PixelGrid_Fall,
			Comp_PixelGrid_Accumulate,
		};

		enum class Material
		{
			Sprite,
			PixelGrid,
			PhysicsDebug,
		};

		enum class Sprite
		{
			Floppa,
			BlueBullet,
		};

		enum class SoundEffect
		{
			SFX,
		};

		enum class PhysicsMaterial
		{
			Defaut,
		};
	}
}
