#pragma once

#include <imgui.h>

namespace REA
{
	class ImGuiHelper
	{
		public:
			static bool ButtonCenteredOnLine(const char* label, float alignment = 0.5f);

			static void CenterNextWindow();
	};
}
