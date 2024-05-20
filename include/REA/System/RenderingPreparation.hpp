#pragma once
#include <SplitEngine/ECS/System.hpp>

using namespace SplitEngine;

namespace REA::System
{
	class RenderingPreparation final : public ECS::SystemBase
	{
		protected:
			void RunExecute(ECS::ContextProvider& contextProvider, uint8_t stage) override;
	};
}
