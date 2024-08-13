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
}
