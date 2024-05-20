#pragma once
#include <ranges>
#include <SplitEngine/ECS/System.hpp>

#include "REA/Component/Physics.hpp"
#include "REA/Component/Transform.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class Physics final : public ECS::System<Component::Transform, Component::Physics>
	{
		public:
			Physics() = default;

		protected:
			void Execute(Component::Transform*  transformComponents,
			             Component::Physics*    physicsComponents,
			             std::vector<uint64_t>& entities,
			             ECS::ContextProvider&  contextProvider,
			             uint8_t                stage) override;

			std::ranges::iota_view<size_t, size_t> _indexes;
	};
}
