#pragma once
#include "Component/AudioSource.hpp"
#include "Component/PixelGrid.hpp"

namespace REA
{
	class PixelGridBuilder
	{
		public:
			PixelGridBuilder() = default;

			PixelGridBuilder&    WithSize(glm::ivec2 size);
			PixelGridBuilder&    WithPixelData(std::vector<Pixel>&& pixelData);
			Component::PixelGrid Build();

		private:
			glm::ivec2         _size = { 0, 0 };
			std::vector<Pixel> _pixels{};
	};
}
