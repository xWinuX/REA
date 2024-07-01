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
			MarchingSquare,
			Comp_PixelGrid_Idle,
			Comp_PixelGrid_Fall,
			Comp_PixelGrid_Accumulate,
			Comp_MarchingSquare,
		};

		enum class Material
		{
			Sprite,
			PixelGrid,
			PhysicsDebug,
			MarchingSquare,
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
