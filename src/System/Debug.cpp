#include "REA/System/Debug.hpp"

#include <imgui.h>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>

#include "REA/Stage.hpp"
#include "REA/Context/ImGui.hpp"

using namespace SplitEngine;

namespace REA::System
{
	Debug::Debug()
	{
		_ecsStageLookup[EngineStage::BeginFrame]     = "Begin Frame";
		_ecsStageLookup[EngineStage::BeginRendering] = "Begin Rendering";
		_ecsStageLookup[EngineStage::EndRendering]   = "End Rendering";
		_ecsStageLookup[Stage::Gameplay]             = "Gameplay";
		_ecsStageLookup[Stage::PrePhysicsStep]       = "Pre Physics";
		_ecsStageLookup[Stage::Physics]              = "Physics";
		_ecsStageLookup[Stage::Rendering]            = "Rendering";
		_ecsStageLookup[Stage::PhysicsManagement]    = "Physics Management";
		_ecsStageLookup[Stage::GridComputeBegin]     = "Grid Compute Begin";
		_ecsStageLookup[Stage::GridComputeHalted]    = "Grid Compute Halted";
		_ecsStageLookup[Stage::GridComputeEnd]       = "Grid Compute End";
		_ecsStageLookup[Stage::LateGameplay]         = "Late Gameplay";
	}

	void Debug::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		if (Input::GetPressed(KeyCode::F3)) { _toggled = !_toggled; }

		if (!_toggled) { return; }

		Context::ImGui* imGuiContext  = contextProvider.GetContext<Context::ImGui>();
		EngineContext*  engineContext = contextProvider.GetContext<EngineContext>();

		ImGui::Begin("Debug");

		if (ImGui::Button("Fullscreen Toggle")) { engineContext->Application->GetWindow().ToggleFullscreen(); }

		Statistics& statistics = engineContext->Statistics;
		ImGui::Text(std::format("{0}: %f", "DeltaTime").c_str(), statistics.AverageDeltaTime);
		ImGui::Text(std::format("{0}: %i", "FPS").c_str(), statistics.AverageFPS);
		ImGui::Text("Frame Time (ms): %f", statistics.AverageDeltaTime / 0.001f);

		std::vector<uint8_t>& activeStages = engineContext->Application->GetECSRegistry().GetActiveStages();

		for (uint8_t activeStage: activeStages) { ImGui::Text("%s: %f", _ecsStageLookup[activeStage].c_str(), statistics.AverageECSStageTimeMs[activeStage]); }

		ImGui::End();
	}
}
