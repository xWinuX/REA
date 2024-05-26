#include "REA/System/PixelGridDrawing.hpp"

#include <imgui.h>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Systems.hpp>

#include "REA/PixelType.hpp"

namespace REA::System
{
	PixelGridDrawing::PixelGridDrawing(int radius):
		_radius(radius) {}

	void PixelGridDrawing::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		ImGui::Begin("Drawing");
		ImGui::SliderInt("Brush Size", &_radius, 1, 50);
		ImGui::End();
		System<Component::PixelGrid>::ExecuteArchetypes(archetypes, contextProvider, stage);
		_mouseWheel = { 0, 0 };
	}

	void PixelGridDrawing::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::PixelGrid& pixelGrid = pixelGrids[i];
			Pixel*                pixels    = pixelGrid.Pixels;

			if (pixels == nullptr) { continue; }

			Pixel drawPixel;

			if (Input::GetDown(KeyCode::MOUSE_LEFT)) { drawPixel = Pixels[PixelType::Wood]; }

			if (Input::GetDown(KeyCode::MOUSE_RIGHT)) { drawPixel = Pixels[PixelType::Water]; }

			if (Input::GetDown(KeyCode::N1)) { drawPixel = Pixels[PixelType::Sand]; }
			if (Input::GetDown(KeyCode::N2)) { drawPixel = Pixels[PixelType::Smoke]; }


			if (Input::GetDown(KeyCode::MOUSE_MIDDLE))
			{
				glm::ivec2 delta = Input::GetMouseDelta();
				pixelGrid.Offset += glm::vec2(-delta.x, delta.y) / pixelGrid.Zoom;
			}


			//if (Input::GetDown(F)) { drawPixel = { 3, BitSet<uint8_t>(Gravity), 3, 1 }; }

			pixelGrid.Zoom += (static_cast<float>(Input::GetMouseWheel().y) * 0.05f) * pixelGrid.Zoom;


			glm::ivec2 windowSize = contextProvider.GetContext<EngineContext>()->Application->GetWindow().GetSize();

			glm::vec2 mousePosition = Input::GetMousePosition();

			// Calculate normalized mouse position
			glm::vec2 offset             = glm::ivec2(pixelGrid.Offset.x, -pixelGrid.Offset.y);
			glm::vec2 normalizedMousePos = glm::vec2((mousePosition / pixelGrid.Zoom) + offset) / glm::vec2(windowSize);

			// Map normalized mouse position to grid position
			int gridX = static_cast<int>(std::round(normalizedMousePos.x * static_cast<float>(pixelGrid.Width)));
			int gridY = static_cast<int>(std::round((static_cast<float>(pixelGrid.Height) / pixelGrid.Zoom) - normalizedMousePos.y * static_cast<float>(pixelGrid.Height)));

			pixelGrid.PointerPosition = { gridX, gridY };

			if (drawPixel.PixelID != 0)
			{
				if (_radius == 1)
				{
					const int xx = std::clamp<int>(gridX, 0, pixelGrid.Width);
					const int yy = std::clamp<int>(gridY, 0, pixelGrid.Height);

					const int index = yy * pixelGrid.Width + xx;
					pixels[index]   = drawPixel;
				}
				else
				{
					for (int x = -_radius; x < _radius; ++x)
					{
						for (int y = -_radius; y < _radius; ++y)
						{
							const int xx    = std::clamp<int>(gridX + x, 0, pixelGrid.Width);
							const int yy    = std::clamp<int>(gridY + y, 0, pixelGrid.Height);
							const int index = yy * pixelGrid.Width + xx;
							if (index < pixelGrid.Width * pixelGrid.Height) { pixels[index] = drawPixel; }
						}
					}
				}
			}
		}
	}
}
