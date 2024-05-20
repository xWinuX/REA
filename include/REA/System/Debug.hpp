#pragma once
#include <SplitEngine/ECS/System.hpp>

namespace REA::System
{
	class Debug final : public SplitEngine::ECS::System<>
	{
		public:
			Debug() = default;

		protected:
			void ExecuteArchetypes(std::vector<SplitEngine::ECS::Archetype*>& archetypes, SplitEngine::ECS::ContextProvider& contextProvider, uint8_t stage) override;
	};
}
