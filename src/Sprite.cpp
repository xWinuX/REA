#include "REA/Sprite.hpp"

namespace REA
{
	Sprite::Sprite(const CreateInfo& createInfo)
	{
		_textureIDs    = std::vector<uint64_t>(createInfo.PackingData.PackMapping[createInfo.PackerID]);
		_numSubSprites = _textureIDs.size();
	}

	size_t Sprite::GetNumSubSprites() const { return _numSubSprites; }

	uint32_t Sprite::GetTextureID(uint32_t index) { return static_cast<uint32_t>(_textureIDs[index]); }
}
