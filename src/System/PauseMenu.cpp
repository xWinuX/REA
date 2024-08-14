#include "REA/System/PauseMenu.hpp"

#include <imgui.h>
#include <REA/ImGuiHelper.hpp>
#include <REA/Stage.hpp>
#include <REA/Component/Camera.hpp>
#include <REA/Context/Global.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>


namespace REA::System
{
	void PauseMenu::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		ECS::Registry& ecs = contextProvider.GetContext<EngineContext>()->Application->GetECSRegistry();
		Context::Global* globalContext = contextProvider.GetContext<Context::Global>();
		if (ecs.GetPrimaryGroup() != Level::MainMenu)
		{
			if (Input::GetPressed(KeyCode::ESCAPE)) { _toggled = !_toggled; }

			if (!_toggled) { return; }

			ImGuiHelper::CenterNextWindow();

			ImGui::Begin("Pause", &_toggled, ImGuiWindowFlags_NoCollapse);

			if (ImGuiHelper::ButtonCenteredOnLine("Continue")) { _toggled = false; }

			if (ImGuiHelper::ButtonCenteredOnLine("Quit to Main Menu"))
			{
				ecs.DestroyGroup(ecs.GetPrimaryGroup());
				ecs.SetPrimaryGroup(Level::MainMenu);

				ecs.GetComponent<Component::Camera>(globalContext->CameraEntityID).TargetEntity = 0ull;

				_toggled = false;
			}

			ImGui::End();
		}
	}
}
