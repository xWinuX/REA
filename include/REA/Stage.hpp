#pragma once
#include <SplitEngine/Stages.hpp>

namespace REA
{
	enum Stage
	{
		//BeginFrame =  20

		PhysicsManagement = 48,

		PrePhysicsStep = 49,

		Physics = 50,

		Gameplay = 100,

		GridComputeEnd = 101,

		GridComputeHalted = 102,

		GridComputeBegin = 103,

		// BeginRendering = 150

		Rendering = 175,

		// EndRendering =  200
	};

	enum Level
	{
		Global   = 0,
		MainMenu = 1,
		Sandbox  = 2,
		Explorer = 3
	};
}
