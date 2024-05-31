#pragma once
#include <imgui.h>

namespace REA::Context
{
	struct ImGui
	{
		ImGuiID TopDockingID = -1u;
		ImGuiID RightDockingID = -1u;
		ImGuiID LeftDockingID = -1u;
		ImGuiID CenterDockingID = -1u;
		ImGuiID TopLeftDockingID = -1u;
	};
}
