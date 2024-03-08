#pragma once
#include <SplitEngine/ECS/System.hpp>

using namespace SplitEngine;

namespace REA::System
{
	class RenderingPreparation final : public ECS::SystemBase
	{
		void RunExecute(ECS::Context& context) override;
	};
}
