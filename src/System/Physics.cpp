#include "REA/System/Physics.hpp"

#include <execution>
#include <imgui.h>
#include <ranges>
#include <SplitEngine/Contexts.hpp>

namespace REA::System
{
	void Physics::Execute(Component::Transform* transformComponents, Component::Physics* physicsComponents, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		_indexes = std::ranges::iota_view(static_cast<size_t>(0), entities.size());

		float deltaTime = contextProvider.GetContext<TimeContext>()->DeltaTime;
		std::for_each(std::execution::par_unseq,
		              _indexes.begin(),
		              _indexes.end(),
		              [transformComponents, physicsComponents, deltaTime](const size_t i) { transformComponents[i].Position += physicsComponents[i].Velocity * deltaTime; });

		ImGui::Text("Num : %llu", entities.size());
	}
}
