#pragma once
#include <array>
#include <SplitEngine/ECS/System.hpp>

namespace REA::System
{
	class Debug final : public SplitEngine::ECS::System<>
	{
		public:
			Debug();

		protected:
			void ExecuteArchetypes(std::vector<SplitEngine::ECS::Archetype*>& archetypes, SplitEngine::ECS::ContextProvider& contextProvider, uint8_t stage) override;

			std::vector<std::string> _ecsStageLookup = std::vector<std::string>(255, "");
	};
}
