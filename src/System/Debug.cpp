#include "REA/System/Debug.hpp"

#include <imgui.h>
#include <SplitEngine/Application.hpp>

using namespace SplitEngine;

namespace REA::System
{
	void Debug::ExecuteArchetypes(std::vector<SplitEngine::ECS::Archetype*>& archetypes, SplitEngine::ECS::Context& context)
	{
		if (ImGui::Button("Fullscreen Toggle")) { context.Application->GetWindow().ToggleFullscreen(); }

		Application::Statistics& statistics = context.Application->GetStatistics();
		ImGui::Text(std::format("{0}: %i", "FPS").c_str(), statistics.AverageFPS);
		ImGui::Text("Frame Time (ms): %f", statistics.AverageDeltaTime / 0.001f);
		ImGui::Text(std::format("{0}: %f", "ECS Prepare (ms)").c_str(), statistics.AverageECSPrepareTime);
		ImGui::Text(std::format("{0}: %f", "ECS Gameplay (ms)").c_str(), statistics.AverageGameplaySystemTime);
		ImGui::Text(std::format("{0}: %f", "ECS Rendering (ms)").c_str(), statistics.AverageRenderSystemTime);
		ImGui::Text(std::format("{0}: %f", "Rendering Begin (ms)").c_str(), statistics.AverageRenderBeginTime);
		ImGui::Text(std::format("{0}: %f", "Rendering End (ms)").c_str(), statistics.AverageRenderEndTime);
	}
}
