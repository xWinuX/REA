#include "REA/System/MainMenu.hpp"

#include <REA/ImGuiHelper.hpp>
#include <REA/PixelGridBuilder.hpp>
#include <REA/Stage.hpp>
#include <REA/Component/Camera.hpp>
#include <REA/Component/PixelGridRenderer.hpp>
#include <REA/Component/Transform.hpp>
#include <REA/Context/Global.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>


namespace REA::Component {
	struct Camera;
}

namespace REA::System
{
	MainMenu::MainMenu(std::vector<PixelGridBuilder::PixelCreateInfo>& pixelCreateInfos):
		_pixelCreateInfos(pixelCreateInfos) {}

	void MainMenu::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		EngineContext* engineContext = contextProvider.GetContext<EngineContext>();

		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		float oldSize = ImGui::GetFont()->Scale;
		ImGui::GetFont()->Scale *= 2.0f;
		ImGui::PushFont(ImGui::GetFont());

		ImGui::Begin("Main Menu", &_open, _flags);

		if (ImGuiHelper::ButtonCenteredOnLine("Start Sandbox"))
		{
			ECS::Registry& ecs = engineContext->Application->GetECSRegistry();

			ecs.SetPrimaryGroup(Level::Sandbox);

			PixelGridBuilder     pixelGridBuilder{};
			Component::PixelGrid pixelGrid = pixelGridBuilder.WithSize({ 8, 8 }, { Constants::CHUNKS_X, Constants::CHUNKS_Y }).WithPixelData(_pixelCreateInfos).Build();
			pixelGrid.CameraEntityID       = contextProvider.GetContext<Context::Global>()->CameraEntityID;

			Component::Camera& camera = ecs.GetComponent<Component::Camera>(pixelGrid.CameraEntityID);

			camera.TargetPosition = glm::vec3((pixelGrid.WorldWidth * 0.5f) * 0.1f, (pixelGrid.WorldHeight * 0.5f) * 0.1f, 0.0f);


			LOG("before entity create");
			ecs.CreateEntity<Component::Transform, Component::PixelGrid, Component::PixelGridRenderer>({}, std::move(pixelGrid), {});
			LOG("after entity create");
		}

		ImGuiHelper::ButtonCenteredOnLine("Start Explorer");

		if (ImGuiHelper::ButtonCenteredOnLine("Quit")) { engineContext->Application->Quit(); }

		ImGui::GetFont()->Scale = oldSize;
		ImGui::PopFont();
		ImGui::End();
	}
}
