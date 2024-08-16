#include "REA/System/PixelGridDrawing.hpp"

#include <execution>
#include <imgui.h>
#include <bitset>
#include <REA/Stage.hpp>

#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>
#include <SplitEngine/Input.hpp>
#include <SplitEngine/Systems.hpp>

#include "IconsFontAwesome.h"
#include "REA/PixelType.hpp"
#include "REA/Component/Transform.hpp"
#include "REA/Component/Camera.hpp"
#include "REA/Context/ImGui.hpp"


namespace REA::System
{
	PixelGridDrawing::PixelGridDrawing(int radius, Pixel::ID clearPixelID):
		_radius(radius),
		_clearPixelID(clearPixelID) {}


	void PixelGridDrawing::ClearGrid(const Component::PixelGrid& pixelGrid)
	{
		Pixel::State pixelState = pixelGrid.PixelLookup[_clearPixelID].PixelState;

		for (int i = 0; i < pixelGrid.Chunks->size(); ++i)
		{
			auto& chunk = (*pixelGrid.Chunks)[pixelGrid.ChunkMapping[i]];

			for (Pixel::State& state: chunk) { state = pixelState; }
		}
	}

	void PixelGridDrawing::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		Context::ImGui* imGuiContext = contextProvider.GetContext<Context::ImGui>();
		ECS::Registry&  ecs          = contextProvider.GetContext<EngineContext>()->Application->GetECSRegistry();

		if (ecs.GetPrimaryGroup() == Level::Sandbox || ecs.GetPrimaryGroup() == Level::Explorer)
		{
			ImGui::SetNextWindowDockID(imGuiContext->RightDockingID, ImGuiCond_Always);
			ImGui::Begin("Drawing", nullptr, ImGuiWindowFlags_NoMove);
			ReaSystem<Component::PixelGrid, Component::PixelGridRenderer>::ExecuteArchetypes(archetypes, contextProvider, stage);
			ImGui::End();
			_mouseWheel = { 0, 0 };
		}
	}

	void PixelGridDrawing::Execute(Component::PixelGrid*         pixelGrids,
	                               Component::PixelGridRenderer* pixelGridRenderers,
	                               std::vector<uint64_t>&        entities,
	                               ECS::ContextProvider&         contextProvider,
	                               uint8_t                       stage)
	{
		EngineContext*  engineContext = contextProvider.GetContext<EngineContext>();
		ECS::Registry&  ecsRegistry   = engineContext->Application->GetECSRegistry();
		Context::ImGui* imGuiContext  = contextProvider.GetContext<Context::ImGui>();

		for (int i = 0; i < entities.size(); ++i)
		{
			Component::PixelGrid&         pixelGrid         = pixelGrids[i];
			Component::PixelGridRenderer& pixelGridRenderer = pixelGridRenderers[i];
			PixelChunks*                  pixelState        = pixelGrid.Chunks;
			std::vector<Pixel>&           pixelLookup       = pixelGrid.PixelLookup;

			if (pixelState == nullptr) { continue; }

			if (pixelGrid.InitialClear)
			{
				ClearGrid(pixelGrid);
				pixelGrid.InitialClear = false;
			}

			// Draw UI
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

			// Get cursor position
			glm::ivec2 cursorPosition = { 0, 0 };
			if (ecsRegistry.IsEntityValid(pixelGrid.CameraEntityID))
			{
				Component::Camera& camera = ecsRegistry.GetComponent<Component::Camera>(pixelGrid.CameraEntityID);

				float pixelSize = camera.PixelSize;

				glm::ivec2 windowSize    = contextProvider.GetContext<EngineContext>()->Application->GetWindow().GetSize();
				glm::ivec2 mousePosition = Input::GetMousePosition();
				mousePosition            = { mousePosition.x, windowSize.y - mousePosition.y };

				glm::ivec2 adjustedWindowSize    = glm::ivec2(glm::floor(glm::vec2(windowSize) / pixelSize));
				glm::ivec2 adjustedMousePosition = glm::ivec2(glm::floor(glm::vec2(mousePosition) / pixelSize));

				Component::Transform& transform = ecsRegistry.GetComponent<Component::Transform>(pixelGrid.CameraEntityID);

				glm::ivec2 cameraPosition        = glm::ivec2(glm::floor(glm::vec2(transform.Position)));
				glm::ivec2 centeredMousePosition = adjustedMousePosition - (adjustedWindowSize / 2);

				cursorPosition = (cameraPosition + centeredMousePosition) - pixelGrid.ChunkOffset * static_cast<int32_t>(Constants::CHUNK_SIZE);
			}

			// Set the pointer position for rendering
			pixelGridRenderer.PointerPosition = cursorPosition;

			// Calculate chunk and pixel index within the chunk
			uint32_t chunkIndex = (cursorPosition.y / Constants::CHUNK_SIZE) * Constants::CHUNKS_X + (cursorPosition.x / Constants::CHUNK_SIZE);
			uint32_t pixelIndex = (cursorPosition.y % Constants::CHUNK_SIZE) * Constants::CHUNK_SIZE + (cursorPosition.x % Constants::CHUNK_SIZE);

			// Skip this frame if chunk index is somehow out of bounds
			if (chunkIndex >= pixelGrid.ChunkMapping.size()) { continue; }

			// Retrieve the chunk mapping to get the correct chunk data
			uint32_t chunkMapping = pixelGrid.ChunkMapping[chunkIndex];

			// Access the pixel state from the pixel state array
			Pixel::State state = (*pixelState)[chunkMapping][pixelIndex];

			// Draw Pixel Info
			ImGui::Text(std::format("Current Pixel Info:").c_str());
			ImGui::Text(std::format("Chunk Index: {0}", chunkIndex).c_str());
			ImGui::Text(std::format("Chunk Pixel Index: {0}", pixelIndex).c_str());
			ImGui::Text(std::format("Pos: x {0} y {1}", cursorPosition.x, cursorPosition.y).c_str());
			ImGui::Text(std::format("ID: {0}", state.PixelID).c_str());
			ImGui::Text(std::format("Name: {0}", pixelGrid.PixelLookup[state.PixelID].Name).c_str());
			ImGui::Text(std::format("Mask: {0}", std::bitset<8>(state.Flags.GetMask()).to_string()).c_str());
			ImGui::Text(std::format("Temperature: {0}", state.Temperature).c_str());
			ImGui::Text(std::format("Charge: {0}", state.Charge).c_str());
			ImGui::Text(std::format("RigidBodyID: {0}", static_cast<uint32_t>(state.RigidBodyID)).c_str());

			// Actually draw the pixels
			if (Input::GetDown(KeyCode::MOUSE_LEFT) && !ImGui::GetIO().WantCaptureMouse)
			{
				Pixel::State& drawState = pixelGrid.PixelLookup[_drawPixelID].PixelState;
				if (_radius == 1)
				{
					(*pixelState)[chunkMapping][pixelIndex] = drawState;
					pixelGrid.ChunkRegenerate[chunkMapping] = true;
				}
				else
				{
					for (int x = -_radius; x < _radius; ++x)
					{
						for (int y = -_radius; y < _radius; ++y)
						{
							if (x * x + y * y > _radius * _radius)  { continue; }

							const int xx = std::clamp<int>(cursorPosition.x + x, 0, pixelGrid.WorldWidth - 1);
							const int yy = std::clamp<int>(cursorPosition.y + y, 0, pixelGrid.WorldHeight - 1);

							chunkIndex = (yy / Constants::CHUNK_SIZE) * Constants::CHUNKS_X + (xx / Constants::CHUNK_SIZE);
							pixelIndex = (yy % Constants::CHUNK_SIZE) * Constants::CHUNK_SIZE + (xx % Constants::CHUNK_SIZE);

							if (chunkIndex < pixelGrid.ChunkMapping.size())
							{
								chunkMapping                            = pixelGrid.ChunkMapping[chunkIndex];
								pixelGrid.ChunkRegenerate[chunkMapping] = true;
								(*pixelState)[chunkMapping][pixelIndex] = drawState;
							}
						}
					}
				}
			}
		}
	}
}
