#pragma once

#include "SplitEngine/Tools/ImagePacker.hpp"

#include <vector>

using namespace SplitEngine;

namespace REA
{
	class SpriteTexture
	{
		public:
			struct CreateInfo
			{
					uint64_t                         PackerID;
					Tools::ImagePacker::PackingData& PackingData;
			};

			 explicit SpriteTexture(const CreateInfo& createInfo);

			[[nodiscard]] size_t GetNumSubSprites() const;

			uint32_t GetTextureID(uint32_t index);

			size_t _numSubSprites;

		private:
			std::vector<uint64_t> _textureIDs;
	};
}
