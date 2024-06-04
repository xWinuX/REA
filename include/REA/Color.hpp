#pragma once
#include <cstdint>
#include <imgui.h>

namespace REA
{
	struct Color
	{
		public:
			Color() = default;
			explicit Color(uint32_t hex);

			float R = 0.0f;
			float G = 0.0f;
			float B = 0.0f;
			float A = 0.0f;

			explicit operator ImVec4() const;

			Color operator * (const float scalar);

		private:
			static float SRGBToLinear(float x);
	};
}
