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

		protected:
			void Execute(Component::Transform*  transformComponents,
			             Component::Camera*     cameraComponents,
			             std::vector<uint64_t>& entities,
			             ECS::ContextProvider&  contextProvider,
			             uint8_t                stage) override;

		private:
			float _pixelsPerUnit = 10.0f;
			float _pixelSize = 1.0f;

			struct CameraUBO
			{
				glm::mat4 identity;
				glm::mat4 view;
				glm::mat4 proj;
				float     pixelSize;
				float     pixelsPerUnit;
			};
	};
}
