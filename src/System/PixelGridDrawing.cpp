#include "REA/System/PixelGridDrawing.hpp"

#include <execution>
#include <imgui.h>
#include <bitset>

#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Systems.hpp>

#include "IconsFontAwesome.h"
#include "REA/PixelType.hpp"
#include "REA/Context/ImGui.hpp"


namespace REA::System
{
	PixelGridDrawing::PixelGridDrawing(int radius, Pixel::ID clearPixelID):
		_radius(radius),
		_clearPixelID(clearPixelID) {}


	void PixelGridDrawing::ClearGrid(const Component::PixelGrid& pixelGrid)
	{
		Pixel::State* inputPixels = pixelGrid.PixelState;

		Pixel::State pixelState = pixelGrid.PixelLookup[_clearPixelID].PixelState;
		std::for_each(std::execution::par_unseq, inputPixels, inputPixels + (pixelGrid.Width * pixelGrid.Height), [this, pixelState](Pixel::State& pixel) { pixel = pixelState; });
	}

	void PixelGridDrawing::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		Context::ImGui* imGuiContext = contextProvider.GetContext<Context::ImGui>();
		ImGui::SetNextWindowDockID(imGuiContext->RightDockingID, ImGuiCond_Always);
		ImGui::Begin("Drawing");
		System<Component::PixelGrid, Component::PixelGridRenderer>::ExecuteArchetypes(archetypes, contextProvider, stage);
		ImGui::End();
		_mouseWheel = { 0, 0 };
	}

	void PixelGridDrawing::Execute(Component::PixelGrid*         pixelGrids,
	                               Component::PixelGridRenderer* pixelGridRenderers,
	                               std::vector<uint64_t>&        entities,
	                               ECS::ContextProvider&         contextProvider,
	                               uint8_t                       stage)
	{
		Context::ImGui* imGuiContext = contextProvider.GetContext<Context::ImGui>();

		for (int i = 0; i < entities.size(); ++i)
		{
			Component::PixelGrid&         pixelGrid         = pixelGrids[i];
			Component::PixelGridRenderer& pixelGridRenderer = pixelGridRenderers[i];
			Pixel::State*                 pixelState        = pixelGrid.PixelState;
			std::vector<Pixel>&           pixelLookup       = pixelGrid.PixelLookup;

			if (pixelState == nullptr) { continue; }

			if (_firstRender)
			{
				ClearGrid(pixelGrid);
				_firstRender = false;
			}

			ImGui::Text("Actions");

			std::string viewMode = "";

			switch (pixelGridRenderer.RenderMode)
			{
				case Component::Normal:
					viewMode = ICON_FA_TEMPERATURE_FULL;
					break;
				case Component::Temperature:
					viewMode = ICON_FA_EYE;
					break;
			}

			if (ImGui::Button(viewMode.c_str(), { 50, 50 }))
			{
				pixelGridRenderer.RenderMode = static_cast<Component::RenderMode>((pixelGridRenderer.RenderMode + 1) % Component::MAX_VALUE);
			}
			ImGui::SameLine();
			IMGUI_COLORED_BUTTON(imGuiContext->ColorDanger, if (ImGui::Button(ICON_FA_TRASH, { 50, 50 })) { ClearGrid(pixelGrid); })


			ImGui::Separator();
			ImGui::SliderInt("Brush Size", &_radius, 1, 100);
			ImGui::Separator();

			ImGui::Spacing();

			ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(3.0f, 3.0f));

			if (ImGui::BeginTable("Pixel Selection", 5))
			{
				for (int i = 0; i < pixelGrid.PixelLookup.size(); ++i)
				{
					Pixel& pixel      = pixelGrid.PixelLookup[i];
					Color& pixelColor = pixelGrid.PixelColorLookup[i];

					ImGui::TableNextColumn();

					ImU32 color    = IM_COL32(pixelColor.R * 255, pixelColor.G * 255, pixelColor.B * 255, pixelColor.A * 255);
					bool  selected = _drawPixelID == pixel.PixelState.PixelID;

					ImGui::GetWindowDrawList()->ChannelsSplit(2);
					ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);


					ImVec2 availableSize = ImGui::GetContentRegionAvail();

					if (ImGui::Button(std::format("{0}", pixel.Name).c_str(), { availableSize.x, 50 })) { _drawPixelID = pixel.PixelState.PixelID; }

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
				pixelGridRenderer.Offset += glm::vec2(-delta.x, delta.y) / pixelGridRenderer.Zoom;
			}

			pixelGridRenderer.Zoom += (static_cast<float>(Input::GetMouseWheel().y) * 0.05f) * pixelGridRenderer.Zoom;

			glm::ivec2 windowSize = contextProvider.GetContext<EngineContext>()->Application->GetWindow().GetSize();

			glm::vec2 mousePosition = Input::GetMousePosition();

			// Calculate normalized mouse position
			glm::vec2 offset             = glm::ivec2(pixelGridRenderer.Offset.x, -pixelGridRenderer.Offset.y);
			glm::vec2 normalizedMousePos = glm::vec2((mousePosition / pixelGridRenderer.Zoom) + offset) / glm::vec2(windowSize);

			// Map normalized mouse position to grid position
			int gridX = static_cast<int>(std::round(normalizedMousePos.x * static_cast<float>(pixelGrid.SimulationWidth)));
			int gridY = static_cast<int>(std::round((static_cast<float>(pixelGrid.SimulationHeight) / pixelGridRenderer.Zoom) - normalizedMousePos.y * static_cast<float>(pixelGrid.SimulationHeight)));

			gridX = static_cast<int>(glm::mod(static_cast<float>(gridX), static_cast<float>(pixelGrid.SimulationWidth)));
			gridY = static_cast<int>(glm::mod(static_cast<float>(gridY), static_cast<float>(pixelGrid.SimulationHeight)));

			pixelGridRenderer.PointerPosition = { gridX, gridY };

			int currentePixelindex = gridY * pixelGrid.Width + gridX;

			Pixel::State state = pixelState[currentePixelindex];

			ImGui::Text(std::format("Current Pixel Info:").c_str());
			ImGui::Text(std::format("Index: {0}", currentePixelindex).c_str());
			ImGui::Text(std::format("Pos: x {0} y {1}", gridX, gridY).c_str());
			ImGui::Text(std::format("ID: {0}", state.PixelID).c_str());
			ImGui::Text(std::format("Name: {0}", pixelGrid.PixelLookup[state.PixelID].Name).c_str());
			ImGui::Text(std::format("Mask: {0}", std::bitset<8>(state.Flags.GetMask()).to_string()).c_str());
			ImGui::Text(std::format("Temperature: {0}", state.Temperature).c_str());
			ImGui::Text(std::format("Charge: {0}", state.Charge).c_str());
			ImGui::Text(std::format("RigidBodyID: {0}", static_cast<uint32_t>(state.RigidBodyID)).c_str());
			//ImGui::Text(std::format("Label: {0}", static_cast<int32_t>(pixelGrid.Labels[currentePixelindex])).c_str());


			if (Input::GetDown(KeyCode::MOUSE_LEFT) && !ImGui::GetIO().WantCaptureMouse)
			{
				Pixel::State& drawState = pixelGrid.PixelLookup[_drawPixelID].PixelState;
				if (_radius == 1)
				{
					const int index   = gridY * pixelGrid.Width + gridX;
					pixelState[index] = drawState;

					//pixelGrid.ReadOnlyPixels[index] = 1;
				}
				else
				{
					for (int x = -_radius; x < _radius; ++x)
					{
						for (int y = -_radius; y < _radius; ++y)
						{
							const int xx    = std::clamp<int>(gridX + x, 0, pixelGrid.SimulationWidth);
							const int yy    = std::clamp<int>(gridY + y, 0, pixelGrid.SimulationHeight);
							const int index = yy * pixelGrid.Width + xx;
							if (index < pixelGrid.Width * pixelGrid.Height)
							{
								//pixelGrid.ReadOnlyPixels[index] = 1;
								pixelState[index] = drawState;
							}
						}
					}
				}
			}
		}
	}
}
