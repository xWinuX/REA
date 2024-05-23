#include "REA/System/Camera.hpp"

#include <imgui.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Shader.hpp>

namespace REA::System
{
	void Camera::Execute(Component::Transform*  transformComponents,
	                     Component::Camera*     cameraComponents,
	                     std::vector<uint64_t>& entities,
	                     ECS::ContextProvider&  contextProvider,
	                     uint8_t                stage)
	{
		CameraUBO* cameraUBO = Rendering::Shader::GetGlobalProperties().GetBufferData<CameraUBO>(0);
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Transform& transformComponent = transformComponents[i];
			Component::Camera&    cameraComponent    = cameraComponents[i];

			if (contextProvider.Registry->IsEntityValid(cameraComponent.TargetEntity))
			{
				transformComponent.Position = contextProvider.Registry->GetComponent<Component::Transform>(cameraComponent.TargetEntity).Position - glm::vec3(0.0f, 0.0f, 10.0f);
			}
			else { transformComponent.Position = glm::vec3(cameraComponent.TargetPosition, 10.0f); }

			Rendering::Renderer* renderer = contextProvider.GetContext<RenderingContext>()->Renderer;

			const uint32_t width  = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetSwapchain().GetExtend().width;
			const uint32_t height = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetSwapchain().GetExtend().height;

			cameraUBO->view         = glm::lookAt(transformComponent.Position, transformComponent.Position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			cameraUBO->viewIdentity = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			cameraUBO->orthoProj = glm::ortho(0.0f, static_cast<float>(4), 0.0f, static_cast<float>(4), 0.01f, 1000.0f);

			cameraUBO->proj = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / -static_cast<float>(height), 0.1f, 1000.0f);

			cameraUBO->viewProj = cameraUBO->proj * cameraUBO->view;

			glm::ivec2 inputMouseOffset = transformComponent.Position;
			inputMouseOffset -= glm::ivec2(static_cast<int>(width / 2u), static_cast<int>(height / 2u));
		}
	}
}
