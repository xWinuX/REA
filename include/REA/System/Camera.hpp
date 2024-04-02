#pragma once
#include <SplitEngine/ECS/System.hpp>
#include <SplitEngine/Rendering/Vulkan/Instance.hpp>

#include "REA/Component/Camera.hpp"
#include "REA/Component/Transform.hpp"

using namespace SplitEngine;

namespace REA::System
{
	class Camera final : public ECS::System<Component::Transform, Component::Camera>
	{
		public:
			Camera() = default;

			void Execute(Component::Transform* transformComponents, Component::Camera* cameraComponents, std::vector<uint64_t>& entities, ECS::Context& context) override;

		private:

			glm::vec3 eye;
			glm::vec3 center;
			glm::vec3 up;

			struct CameraUBO
			{
				glm::mat4 view;
				glm::mat4 viewIdentity;
				glm::mat4 proj;
				glm::mat4 orthoProj;
				glm::mat4 viewProj;
			};
	};
}
