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

			PixelGridBuilder&    WithSize(glm::ivec2 worldChunkSize, glm::ivec2 simulationChunkSize);
			PixelGridBuilder&    WithPixelData(std::vector<PixelCreateInfo>&& pixelData);
			Component::PixelGrid Build();

		private:
			glm::ivec2                   _worldChunkSize      = { 0, 0 };
			glm::ivec2                   _simulationChunkSize = { 0, 0 };
			std::vector<PixelCreateInfo> _pixelCreateInfos{};
	};
}
