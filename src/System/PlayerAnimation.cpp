#include "REA/System/PlayerAnimation.hpp"

namespace REA::System
{
	void PlayerAnimation::Execute(Component::Player*         players,
	                              Component::SpriteRenderer* spriteRenderers,
	                              std::vector<uint64_t>&     entities,
	                              ECS::ContextProvider&      context,
	                              uint8_t                    stage)
	{
		for (int entityIndex = 0; entityIndex < entities.size(); ++entityIndex)
		{
			Component::Player          player         = players[entityIndex];
			Component::SpriteRenderer& spriteRenderer = spriteRenderers[entityIndex];

			float length = glm::length(player.Velocity);
			if (length > 1.0f)
			{
				player.FaceDirection = static_cast<int>(glm::sign(player.Velocity.x));

				spriteRenderer.AnimationSpeed = length * 0.1f;

				if (player.FaceDirection < 0) { spriteRenderer.SpriteTexture = player.WalkSpriteL; }
				else { spriteRenderer.SpriteTexture = player.WalkSpriteR; }
			}
			else
			{
				if (player.FaceDirection < 0) { spriteRenderer.SpriteTexture = player.IdleSpriteL; }
				else { spriteRenderer.SpriteTexture = player.IdleSpriteR; }
			}
		}
	}
}
