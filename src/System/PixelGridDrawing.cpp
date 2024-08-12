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
#include "REA/Component/Transform.hpp"
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

		//	Pixel::State pixelState = pixelGrid.PixelLookup[_clearPixelID].PixelState;
		//	std::for_each(std::execution::par_unseq, inputPixels, inputPixels + (pixelGrid.Width * pixelGrid.Height), [this, pixelState](Pixel::State& pixel) { pixel = pixelState; });
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


			glm::ivec2 windowSize = contextProvider.GetContext<EngineContext>()->Application->GetWindow().GetSize();

			glm::vec2 mousePosition = Input::GetMousePosition();
			mousePosition           = { mousePosition.x, windowSize.y - mousePosition.y };


			int gridX = 0;
			int gridY = 0;

			// Map normalized mouse position to grid position
			if (ecsRegistry.IsEntityValid(pixelGrid.CameraEntityID))
			{
				Component::Transform& transform = ecsRegistry.GetComponent<Component::Transform>(pixelGrid.CameraEntityID);

				// Precompute common terms
				int halfChunkSize = Constants::CHUNK_SIZE / 2;
				int gridCenterX = (Constants::CHUNKS_X / 2) * Constants::CHUNK_SIZE + halfChunkSize;
				int gridCenterY = (Constants::CHUNKS_Y / 2) * Constants::CHUNK_SIZE + halfChunkSize;

				// Compute gridX and gridY
				int32_t offsetX = static_cast<int32_t>((transform.Position.x * 10.0f) - halfChunkSize) % Constants::CHUNK_SIZE;
				int32_t offsetY = static_cast<int32_t>((transform.Position.y * 10.0f) - halfChunkSize) % Constants::CHUNK_SIZE;

				gridX = gridCenterX + offsetX;
				gridY = gridCenterY + offsetY;

				// Adjust with mouse position
				mousePosition += glm::vec2(transform.Position);

				gridX += static_cast<int>(mousePosition.x) - static_cast<int>(transform.Position.x + (static_cast<float>(windowSize.x) / 2.0f));
				gridY += static_cast<int>(mousePosition.y) - static_cast<int>(transform.Position.y + (static_cast<float>(windowSize.y) / 2.0f));
			}


			// Ensure the grid position is within the bounds
			gridX = std::clamp(gridX, 0, pixelGrid.SimulationWidth - 1);
			gridY = std::clamp(gridY, 0, pixelGrid.SimulationHeight - 1);

			// Set the pointer position for rendering
			pixelGridRenderer.PointerPosition = { gridX, gridY };

			// Calculate chunk and pixel index within the chunk
			uint32_t chunkIndex = (gridY / Constants::CHUNK_SIZE) * Constants::CHUNKS_X + (gridX / Constants::CHUNK_SIZE);
			uint32_t pixelIndex = (gridY % Constants::CHUNK_SIZE) * Constants::CHUNK_SIZE + (gridX % Constants::CHUNK_SIZE);

			if (chunkIndex < pixelGrid.ChunkMapping.size())
			{
				// Retrieve the chunk mapping to get the correct chunk data
				uint32_t chunkMapping = pixelGrid.ChunkMapping[chunkIndex];

				// Access the pixel state from the pixel state array
				Pixel::State state = (*pixelState)[chunkMapping][pixelIndex];

				ImGui::Text(std::format("Current Pixel Info:").c_str());
				ImGui::Text(std::format("Chunk Index: {0}", chunkIndex).c_str());
				ImGui::Text(std::format("Chunk Pixel Index: {0}", pixelIndex).c_str());
				ImGui::Text(std::format("Pos: x {0} y {1}", gridX, gridY).c_str());
				ImGui::Text(std::format("ID: {0}", state.PixelID).c_str());
				ImGui::Text(std::format("Name: {0}", pixelGrid.PixelLookup[state.PixelID].Name).c_str());
				ImGui::Text(std::format("Mask: {0}", std::bitset<8>(state.Flags.GetMask()).to_string()).c_str());
				ImGui::Text(std::format("Temperature: {0}", state.Temperature).c_str());
				ImGui::Text(std::format("Charge: {0}", state.Charge).c_str());
				ImGui::Text(std::format("RigidBodyID: {0}", static_cast<uint32_t>(state.RigidBodyID)).c_str());

				if (Input::GetDown(KeyCode::MOUSE_LEFT) && !ImGui::GetIO().WantCaptureMouse)
				{
					Pixel::State& drawState = pixelGrid.PixelLookup[_drawPixelID].PixelState;
					if (_radius == 1)
					{
						//const int index   = gridY * pixelGrid.Width + gridX;
						(*pixelState)[chunkMapping][pixelIndex] = drawState;
						pixelGrid.ChunkRegenerate[chunkMapping] = true;

						//pixelGrid.ReadOnlyPixels[index] = 1;
					}
					else
					{
						for (int x = -_radius; x < _radius; ++x)
						{
							for (int y = -_radius; y < _radius; ++y)
							{
								const int xx = std::clamp<int>(gridX + x, 0, pixelGrid.WorldWidth - 1);
								const int yy = std::clamp<int>(gridY + y, 0, pixelGrid.WorldHeight - 1);

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
}
