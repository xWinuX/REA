#pragma once
#include <cstdint>

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

		private:
			static float SRGBToLinear(float x);
	};
}
