#pragma once
#include <REA/Component/Camera.hpp>
#include <SplitEngine/ECS/Archetype.hpp>
#include <SplitEngine/ECS/ContextProvider.hpp>
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/SandboxController.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class SandboxController : public ECS::System<Component::Camera, Component::SandboxController>
	{
		protected:
			void Execute(Component::Camera*            cameras,
			             Component::SandboxController* sandboxControllers,
			             std::vector<uint64_t>&        entities,
			             ECS::ContextProvider&         context,
			             uint8_t                       stage) override;
	};
}
