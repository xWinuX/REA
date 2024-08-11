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
			Comp_PixelGrid_RigidBody,
			Comp_PixelGrid_RigidBodyRemove,
			Comp_PixelGrid_Fall,
			Comp_PixelGrid_Accumulate,
			Comp_PixelGrid_Particle,
			Comp_MarchingSquare,
			Comp_CCLInitialize,
			Comp_CCLColumn,
			Comp_CCLMerge,
			Comp_CCLRelabel,
			Comp_CCLExtract,
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
			Rea_Idle_R
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
