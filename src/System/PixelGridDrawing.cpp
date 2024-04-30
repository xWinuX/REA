#include "REA/System/PixelGridDrawing.hpp"

#include <SplitEngine/Application.hpp>
#include <SplitEngine/Input.hpp>

namespace REA::System
{
	PixelGridDrawing::PixelGridDrawing(int radius):
		_radius(radius) {}

	void PixelGridDrawing::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::Context& context)
	{
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::PixelGrid& pixelGrid = pixelGrids[i];
			Pixel*                pixels    = pixelGrid.Pixels;

			if (pixels == nullptr) { continue; }

			Pixel drawPixel;
			if (Input::GetDown(MOUSE_LEFT)) { drawPixel = { 1, BitSet<uint8_t>(Gravity), 3, 0 }; }

			if (Input::GetDown(MOUSE_RIGHT)) { drawPixel = { 2, BitSet<uint8_t>(Gravity), 2, 1 }; }

			if (drawPixel.PixelID != 0)
			{
				glm::ivec2 windowSize = context.Application->GetWindow().GetSize();

				glm::ivec2 mousePosition = Input::GetMousePosition() + windowSize / 2;


				// Calculate normalized mouse position
				glm::vec2 normalizedMousePos = glm::vec2(mousePosition) / glm::vec2(windowSize);

				// Map normalized mouse position to grid position
				int gridX = static_cast<int>(normalizedMousePos.x * pixelGrid.Width);
				int gridY = pixelGrid.Height - static_cast<int>(normalizedMousePos.y * pixelGrid.Height);

				if (_radius == 1)
				{
					const int xx    = std::clamp<int>(gridX, 0, pixelGrid.Width);
					const int yy    = std::clamp<int>(gridY, 0, pixelGrid.Height);
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
