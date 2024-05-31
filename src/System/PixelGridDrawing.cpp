#include "REA/System/PixelGridDrawing.hpp"

#include <imgui.h>

#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Systems.hpp>

#include "REA/PixelType.hpp"
#include "REA/Context/ImGui.hpp"


namespace REA::System
{
	PixelGridDrawing::PixelGridDrawing(int radius):
		_radius(radius) {}

	void PixelGridDrawing::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		Context::ImGui* imGuiContext = contextProvider.GetContext<Context::ImGui>();
		ImGui::SetNextWindowDockID(imGuiContext->RightDockingID, ImGuiCond_Always);
		ImGui::Begin("Drawing");
		ImGui::SliderInt("Brush Size", &_radius, 1, 50);
		System<Component::PixelGrid>::ExecuteArchetypes(archetypes, contextProvider, stage);
		ImGui::End();
		_mouseWheel = { 0, 0 };
	}

	void PixelGridDrawing::Execute(Component::PixelGrid* pixelGrids, std::vector<uint64_t>& entities, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		for (int i = 0; i < entities.size(); ++i)
		{
			Component::PixelGrid& pixelGrid   = pixelGrids[i];
			Pixel::Data*          pixels      = pixelGrid.PixelData;
			std::vector<Pixel>&   pixelLookup = pixelGrid.PixelLookup;

			if (pixels == nullptr) { continue; }

			ImGui::Spacing();

			ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(3.0f, 3.0f));

			if (ImGui::BeginTable("Pixel Selection", 5))
			{
				for (Pixel& pixel: pixelLookup)
				{
					ImGui::TableNextColumn();

					ImU32 color      = IM_COL32(pixel.Color.R * 255, pixel.Color.G * 255, pixel.Color.B * 255, pixel.Color.A * 255);
					bool  selected = _drawPixel != nullptr && _drawPixel->PixelData.PixelID == pixel.PixelData.PixelID;

					ImGui::GetWindowDrawList()->ChannelsSplit(2);
					ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);


					ImVec2 availableSize = ImGui::GetContentRegionAvail();

					if (ImGui::Button(std::format("{0}", pixel.Name).c_str(), {availableSize.x, 50})) { _drawPixel = &pixel; }

					ImVec2 p_min = ImGui::GetItemRectMin();
					ImVec2 p_max = ImGui::GetItemRectMax();

					if (selected) { ImGui::GetWindowDrawList()->AddRect(p_min, p_max, 0xFFFFFFFF, 3.0f, 0, 3.0f); }

					// Render background behind Selectable().
					ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
					ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, color, 3.0f);
					ImGui::GetWindowDrawList()->ChannelsMerge();


				}
				ImGui::EndTable();
			}
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
			ImGui::Separator();

			if (Input::GetDown(KeyCode::MOUSE_MIDDLE))
			{
				glm::ivec2 delta = Input::GetMouseDelta();
				pixelGrid.Offset += glm::vec2(-delta.x, delta.y) / pixelGrid.Zoom;
			}

			pixelGrid.Zoom += (static_cast<float>(Input::GetMouseWheel().y) * 0.05f) * pixelGrid.Zoom;

			glm::ivec2 windowSize = contextProvider.GetContext<EngineContext>()->Application->GetWindow().GetSize();

			glm::vec2 mousePosition = Input::GetMousePosition();

			// Calculate normalized mouse position
			glm::vec2 offset             = glm::ivec2(pixelGrid.Offset.x, -pixelGrid.Offset.y);
			glm::vec2 normalizedMousePos = glm::vec2((mousePosition / pixelGrid.Zoom) + offset) / glm::vec2(windowSize);

			// Map normalized mouse position to grid position
			int gridX = static_cast<int>(std::round(normalizedMousePos.x * static_cast<float>(pixelGrid.Width)));
			int gridY = static_cast<int>(std::round((static_cast<float>(pixelGrid.Height) / pixelGrid.Zoom) - normalizedMousePos.y * static_cast<float>(pixelGrid.Height)));

			gridX = static_cast<int>(glm::mod(static_cast<float>(gridX), static_cast<float>(pixelGrid.Width)));
			gridY = static_cast<int>(glm::mod(static_cast<float>(gridY), static_cast<float>(pixelGrid.Height)));

			pixelGrid.PointerPosition = { gridX, gridY };

			if (Input::GetDown(KeyCode::MOUSE_LEFT) && _drawPixel != nullptr)
			{
				Pixel::Data& pixelData = _drawPixel->PixelData;
				if (_radius == 1)
				{
					const int index = gridY * pixelGrid.Width + gridX;
					pixels[index]   = pixelData;
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
							if (index < pixelGrid.Width * pixelGrid.Height) { pixels[index] = pixelData; }
						}
					}
				}
			}
		}
	}
}
