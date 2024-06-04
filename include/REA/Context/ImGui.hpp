#pragma once
#include <imgui.h>

#define IMGUI_COLORED_BUTTON(colorName, func) \
	ImGui::PushStyleColor(ImGuiCol_Button, colorName);\
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorName##Hover);\
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorName##Clicked);\
	func \
	ImGui::PopStyleColor(); \
	ImGui::PopStyleColor(); \
	ImGui::PopStyleColor(); \

namespace REA::Context
{
	struct ImGui
	{
		ImGuiID TopDockingID     = -1u;
		ImGuiID RightDockingID   = -1u;
		ImGuiID LeftDockingID    = -1u;
		ImGuiID CenterDockingID  = -1u;
		ImGuiID TopLeftDockingID = -1u;

		ImVec4 ColorDanger{};
		ImVec4 ColorDangerHover{};
		ImVec4 ColorDangerClicked{};
		ImVec4 ColorSuccess{};
	};
}
