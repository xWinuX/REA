#include "REA/System/PixelGridDrawing.hpp"

#include <SplitEngine/Application.hpp>
#include <SplitEngine/Input.hpp>

namespace REA::System
{

	void PixelGridDrawing::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::Context& context)
	{
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::PixelGrid& pixelGrid = pixelGrids[i];
			std::vector<Pixel>& pixels = pixelGrid.Pixels;

			Pixel drawPixel;
			if (Input::GetPressed(MOUSE_LEFT))
			{
				drawPixel = {1, BitSet<uint8_t>(Gravity), 2, 0};
			}

			if (Input::GetPressed(MOUSE_RIGHT))
			{
				drawPixel = {2, BitSet<uint8_t>(Gravity), 1, 1};
			}

			if (Input::GetDown(N1))
			{
				drawPixel = {3, BitSet<uint8_t>(Gravity), -1, 1};
			}

			if (Input::GetPressed(KeyCode::F))
			{
				for (Pixel& pixel : pixels)
				{
					pixel.PixelID = 2;
				}
			}


			if (drawPixel.PixelID != 0)
			{
				glm::ivec2 windowSize = context.Application->GetWindow().GetSize();

				glm::ivec2 mousePosition = Input::GetMousePosition() + windowSize/2;


				// Calculate normalized mouse position
				glm::vec2 normalizedMousePos = glm::vec2(mousePosition) / glm::vec2(windowSize);

				// Map normalized mouse position to grid position
				int gridX = static_cast<int>(normalizedMousePos.x * pixelGrid.Width);
				int gridY = pixelGrid.Height - static_cast<int>(normalizedMousePos.y * pixelGrid.Height);

				LOG("x: {0}, y: {1}", mousePosition.x, mousePosition.y);
				LOG("x: {0}, y: {1}", gridX, gridY);

				const int radius = 10;


				const int xx    = std::clamp<int>(gridX, 0, pixelGrid.Width);
				const int yy    = std::clamp<int>(gridY, 0, pixelGrid.Height);
				const int index = yy * pixelGrid.Width + xx;
				pixels[index] = drawPixel;


/*
				for (int x = -radius; x < radius; ++x)
				{
					for (int y = -radius; y < radius; ++y)
					{
						const int xx    = std::clamp<int>(gridX + x, 0, pixelGrid.Width);
						const int yy    = std::clamp<int>(gridY + y, 0, pixelGrid.Height);
						const int index = yy * pixelGrid.Width + xx;
						if (index < pixels.size())
						{
							pixels[index] = drawPixel;
						}
					}
				}*/

			}
		}
	}
}
