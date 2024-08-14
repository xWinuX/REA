#pragma once
#include <SplitEngine/ECS/System.hpp>

using namespace SplitEngine;

namespace REA:: System
{
	class PauseMenu final : public ECS::System<>
	{
		protected:
			void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;

		private:
			bool _toggled = false;
	};
}
