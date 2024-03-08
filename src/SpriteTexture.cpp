#include "REA/SpriteTexture.hpp"

namespace REA
{
	SpriteTexture::SpriteTexture(const CreateInfo& createInfo)
	{
		_textureIDs    = std::vector<uint64_t>(createInfo.PackingData.PackMapping[createInfo.PackerID]);
		_numSubSprites = _textureIDs.size();
	}

	size_t SpriteTexture::GetNumSubSprites() const { return _numSubSprites; }

	uint32_t SpriteTexture::GetTextureID(uint32_t index) { return static_cast<uint32_t>(_textureIDs[index]); }
}
