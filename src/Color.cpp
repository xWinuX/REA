#include "REA/Color.hpp"

#include <SplitEngine/Debug/Log.hpp>

#include <glm/exponential.hpp>

namespace REA
{
	Color::Color(const uint32_t hex) :
		R(SRGBToLinear(static_cast<float>((hex >> 24) & 0xFFu) / 255.0f)),
		G(SRGBToLinear(static_cast<float>((hex >> 16) & 0xFFu) / 255.0f)),
		B(SRGBToLinear(static_cast<float>((hex >> 8) & 0xFFu) / 255.0f)),
		A(SRGBToLinear(static_cast<float>(hex & 0xFFu) / 255.0f)) { LOG("{0} {1} {2} {3}", R, G, B, A); }

	float Color::SRGBToLinear(float x) { return x <= 0.04045f ? x / 12.92f : glm::pow((x + 0.055f) / 1.055f, 2.4f); }
}