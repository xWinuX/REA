#include "REA/System/Camera.hpp"

#include <imgui.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Rendering/Renderer.hpp>
#include <SplitEngine/Rendering/Shader.hpp>

namespace REA::System
{
	// Source: https://github.com/PacktPublishing/Vulkan-Cookbook/blob/master/Library/Source%20Files/10%20Helper%20Recipes/05%20Preparing%20an%20orthographic%20projection%20matrix.cpp
	glm::mat4 Camera::PrepareOrthographicProjectionMatrix(float leftPlane, float rightPlane, float bottomPlane, float topPlane, float nearPlane, float farPlane)
	{
		glm::mat4 orthographic_projection_matrix = {
			2.0f / (rightPlane - leftPlane),
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			2.0f / (bottomPlane - topPlane),
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			1.0f / (nearPlane - farPlane),
			0.0f,
			-(rightPlane + leftPlane) / (rightPlane - leftPlane),
			-(bottomPlane + topPlane) / (bottomPlane - topPlane),
			nearPlane / (nearPlane - farPlane),
			1.0f
		};
		return orthographic_projection_matrix;
	}

	void Camera::Execute(Component::Transform*  transformComponents,
	                     Component::Camera*     cameraComponents,
	                     std::vector<uint64_t>& entities,
	                     ECS::ContextProvider&  contextProvider,
	                     uint8_t                stage)
	{
		ECS::Registry& ecs = contextProvider.GetContext<EngineContext>()->Application->GetECSRegistry();
		CameraUBO* cameraUBO = Rendering::Shader::GetGlobalProperties().GetBufferData<CameraUBO>(0);
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Transform& transformComponent = transformComponents[i];
			Component::Camera&    cameraComponent    = cameraComponents[i];

			if (cameraComponent.TargetEntity != -1ull && ecs.IsEntityValid(cameraComponent.TargetEntity))
			{
				Component::Transform& targetTransform = contextProvider.Registry->GetComponent<Component::Transform>(cameraComponent.TargetEntity);
				glm::vec2             newPosition     = glm::mix(glm::vec2(transformComponent.Position), glm::vec2(targetTransform.Position.x, targetTransform.Position.y), 0.125f);

				cameraComponent.TargetPosition = glm::vec3(newPosition, cameraComponent.Layer);
			}

			transformComponent.Position = glm::vec3(glm::mix(glm::vec2(transformComponent.Position), glm::vec2(cameraComponent.TargetPosition), 0.25f), cameraComponent.Layer);

			Rendering::Renderer* renderer = contextProvider.GetContext<RenderingContext>()->Renderer;

			const uint32_t width  = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetSwapchain().GetExtend().width;
			const uint32_t height = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetSwapchain().GetExtend().height;

			float left   = 0.0f;
			float right  = static_cast<float>(width) / (_pixelSize * _pixelsPerUnit);
			float bottom = static_cast<float>(height) / (_pixelSize * _pixelsPerUnit);
			float top    = 0.0f;

			glm::vec3 windowSize = glm::vec3(right, bottom, 0.0f);
			cameraUBO->view      = glm::translate(glm::identity<glm::mat4>(), -transformComponent.Position + windowSize / 2.0f);
			cameraUBO->proj      = PrepareOrthographicProjectionMatrix(left, right, bottom, top, 0.1f, 1000.0f);

			cameraUBO->pixelSize     = _pixelSize;
			cameraUBO->pixelsPerUnit = _pixelsPerUnit;
		}
	}
}
