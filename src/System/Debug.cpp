#include "REA/System/Debug.hpp"

#include <imgui.h>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>

#include "REA/Context/ImGui.hpp"

using namespace SplitEngine;

namespace REA::System
{
	void Debug::ExecuteArchetypes(std::vector<SplitEngine::ECS::Archetype*>& archetypes, SplitEngine::ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		Context::ImGui* imGuiContext = contextProvider.GetContext<Context::ImGui>();
		ImGui::SetNextWindowDockID(imGuiContext->TopLeftDockingID, ImGuiCond_Always);
		ImGui::Begin("Debug");
		EngineContext* engineContext = contextProvider.GetContext<EngineContext>();

		if (ImGui::Button("Fullscreen Toggle")) { engineContext->Application->GetWindow().ToggleFullscreen(); }

		Statistics& statistics = engineContext->Statistics;
		ImGui::Text(std::format("{0}: %f", "DeltaTime").c_str(), statistics.AverageDeltaTime);
		ImGui::Text(std::format("{0}: %i", "FPS").c_str(), statistics.AverageFPS);
		ImGui::Text("Frame Time (ms): %f", statistics.AverageDeltaTime / 0.001f);
		ImGui::End();
	}
}
