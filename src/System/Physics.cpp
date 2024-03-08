#include "REA/System/Physics.hpp"

#include <execution>
#include <imgui.h>
#include <ranges>

namespace REA::System
{
	void Physics::Execute(Component::Transform* transformComponents, Component::Physics* physicsComponents, std::vector<uint64_t>& entities, ECS::Context& context)
	{
		_indexes = std::ranges::iota_view(static_cast<size_t>(0), entities.size());
		std::for_each(std::execution::par,
		              _indexes.begin(),
		              _indexes.end(),
		              [transformComponents, physicsComponents, entities, context](const size_t i)
		              {
			              transformComponents[i].Position += physicsComponents[i].Velocity * context.DeltaTime;
		              });

		ImGui::Text("Num : %llu", entities.size());
	}
}
