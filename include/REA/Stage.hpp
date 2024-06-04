#pragma once
#include <SplitEngine/Stages.hpp>

namespace REA
{
	enum Stage
	{
		//BeginFrame =  20

		Gameplay = 100,

		GridComputeEnd = 101,

		GridComputeHalted = 102,

		GridComputeBegin = 103,

		// BeginRendering = 150

		Rendering = 175,

		// EndRendering =  200
	};
}
