#pragma once
#include <REA/Component/Player.hpp>
#include <REA/Component/SpriteRenderer.hpp>
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Vulkan/Allocator.hpp>

using namespace SplitEngine;

namespace REA::System
{
	class PlayerAnimation final : public ECS::System<Component::Player, Component::SpriteRenderer>
	{
		protected:
			void Execute(Component::Player*         players,
			             Component::SpriteRenderer* spriteRenderers,
			             std::vector<uint64_t>&     entities,
			             ECS::ContextProvider&      context,
			             uint8_t                    stage) override;
	};
}
