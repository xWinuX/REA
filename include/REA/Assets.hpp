#pragma once

namespace REA
{
	enum class InputAction
	{
		Move,
		Rotate,
		Fire,
	};

	enum class Shader
	{
		Sprite,
		PixelGrid,
		Comp_PixelGrid_Idle,
		Comp_PixelGrid_Fall,
		Comp_PixelGrid_Accumulate,
	};

	enum class Material
	{
		Sprite,
		PixelGrid,
	};

	enum class Texture
	{
		Carsten,
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
}
