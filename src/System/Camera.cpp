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
	glm::mat4 PrepareOrthographicProjectionMatrix(float left_plane, float right_plane, float bottom_plane, float top_plane, float near_plane, float far_plane)
	{
		glm::mat4 orthographic_projection_matrix = {
			2.0f / (right_plane - left_plane),
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			2.0f / (bottom_plane - top_plane),
			0.0f,
			0.0f,
			0.0f,
			0.0f,
			1.0f / (near_plane - far_plane),
			0.0f,
			-(right_plane + left_plane) / (right_plane - left_plane),
			-(bottom_plane + top_plane) / (bottom_plane - top_plane),
			near_plane / (near_plane - far_plane),
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
		CameraUBO* cameraUBO = Rendering::Shader::GetGlobalProperties().GetBufferData<CameraUBO>(0);
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Transform& transformComponent = transformComponents[i];
			Component::Camera&    cameraComponent    = cameraComponents[i];

			if (contextProvider.Registry->IsEntityValid(cameraComponent.TargetEntity))
			{
				Component::Transform& targetTransform = contextProvider.Registry->GetComponent<Component::Transform>(cameraComponent.TargetEntity);
				transformComponent.Position           = glm::vec3(targetTransform.Position.x, targetTransform.Position.y, transformComponent.Position.z);
			}
			else { transformComponent.Position = glm::vec3(cameraComponent.TargetPosition, 0.0f); }

			Rendering::Renderer* renderer = contextProvider.GetContext<RenderingContext>()->Renderer;

			//cameraUBO->proj = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 0.1f, 1000.0f);
			//cameraUBO->identity = glm::identity<glm::mat4>();
			//cameraUBO->view = glm::lookAt(transformComponent.Position, transformComponent.Position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			//cameraUBO->proj = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / -static_cast<float>(height), 0.1f, 1000.0f) * cameraUBO->view;


			ImGui::SliderFloat("pixelsPerUnit", &_pixelsPerUnit, 0.1f, 100.0f);
			ImGui::SliderFloat("pixelSize", &_pixelSize, 0.1f, 50.0f);

			const uint32_t width  = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetSwapchain().GetExtend().width;
			const uint32_t height = renderer->GetVulkanInstance().GetPhysicalDevice().GetDevice().GetSwapchain().GetExtend().height;

			float left   = 0.0f;
			float right  = static_cast<float>(width) / (_pixelSize);
			float bottom = static_cast<float>(height) / (_pixelSize);
			float top    = 0.0f;

			glm::vec3 windowSize = glm::vec3(right, bottom, 0.0f);
			cameraUBO->view      = glm::translate(glm::identity<glm::mat4>(), -transformComponent.Position + windowSize / 2.0f);
			cameraUBO->proj      = PrepareOrthographicProjectionMatrix(left, right, bottom, top, 0.1f, 1000.0f);

			cameraUBO->pixelSize     = _pixelSize;
			cameraUBO->pixelsPerUnit = _pixelsPerUnit;

			//cameraUBO->proj = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / -static_cast<float>(height), 0.1f, 1000.0f);
		}
	}
}
