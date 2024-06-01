#pragma once
#include "Component/AudioSource.hpp"
#include "Component/PixelGrid.hpp"

namespace REA
{
	class PixelGridBuilder
	{
		public:
			PixelGridBuilder() = default;

			struct PixelCreateInfo
			{
				Pixel::ID   ID   = -1u;
				std::string Name = "NAME_HERE";
				Color       Color{};
				Pixel::Data Data{};
			};

			PixelGridBuilder&    WithSize(glm::ivec2 size);
			PixelGridBuilder&    WithPixelData(std::vector<PixelCreateInfo>&& pixelData);
			Component::PixelGrid Build();

		private:
			glm::ivec2                   _size = { 0, 0 };
			std::vector<PixelCreateInfo> _pixelCreateInfos{};
	};
}
