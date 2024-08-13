#pragma once
#include <SplitEngine/ECS/System.hpp>

#include <imgui.h>
#include <REA/PixelGridBuilder.hpp>

using namespace SplitEngine;

namespace REA
{
	namespace System
	{
		class MainMenu final : public ECS::System<>
		{
			public:
				MainMenu(std::vector<PixelGridBuilder::PixelCreateInfo>& pixelCreateInfos);

			protected:
				void ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage) override;

			private:
				bool             _open  = true;
				ImGuiWindowFlags _flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground |
				                          ImGuiWindowFlags_NoTitleBar;

				std::vector<PixelGridBuilder::PixelCreateInfo>& _pixelCreateInfos;

				std::string _startSandboxText  = "Start Sandbox";
				std::string _startExplorerText = "Start Explorer";
				std::string _quitText          = "Quit";
		};
	}
}
