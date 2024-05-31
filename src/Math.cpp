#include "REA/Math.hpp"

namespace REA
{
	float Math::FastFmod(float a, float b) { return ((a) - ((int)((a) / (b))) * (b)); }
} // REA
