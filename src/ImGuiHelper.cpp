#include "REA/ImGuiHelper.hpp"

namespace REA
{
	bool ImGuiHelper::ButtonCenteredOnLine(const char* label, float alignment)
	{
		ImGuiStyle& style = ImGui::GetStyle();

		float size      = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
		float available = ImGui::GetContentRegionAvail().x;

		float off = (available - size) * alignment;
		if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

		return ImGui::Button(label);
	}

	void ImGuiHelper::CenterNextWindow()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	}
}
