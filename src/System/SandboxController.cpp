#include "REA/System/SandboxController.hpp"

#include <SplitEngine/Input.hpp>


namespace REA::System
{
	void SandboxController::Execute(Component::Camera*            cameras,
	                                Component::SandboxController* sandboxControllers,
	                                std::vector<uint64_t>&        entities,
	                                ECS::ContextProvider&         context,
	                                uint8_t                       stage)
	{
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::Camera&            camera            = cameras[i];
			Component::SandboxController& sandboxController = sandboxControllers[i];

			float mouseWheel = static_cast<float>(Input::GetMouseWheel().y) * sandboxController.ZoomSpeed;

			camera.PixelSize = glm::clamp(camera.PixelSize + mouseWheel, 0.1f, 50.0f);

			if (Input::GetDown(KeyCode::MOUSE_MIDDLE))
			{
				glm::ivec2 mouseDelta = Input::GetMouseDelta();
				camera.TargetPosition += glm::vec2(-mouseDelta.x, mouseDelta.y) / camera.PixelSize;
			}
		}
	}
}
