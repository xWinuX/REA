#include "REA/System/MainMenu.hpp"

#include <box2d/b2_polygon_shape.h>
#include <REA/Assets.hpp>
#include <REA/ImGuiHelper.hpp>
#include <REA/PixelGridBuilder.hpp>
#include <REA/PixelType.hpp>
#include <REA/Stage.hpp>
#include <REA/WorldGenerator.hpp>
#include <REA/Component/Camera.hpp>
#include <REA/Component/Collider.hpp>
#include <REA/Component/PixelGridRenderer.hpp>
#include <REA/Component/Player.hpp>
#include <REA/Component/SpriteRenderer.hpp>
#include <REA/Component/Transform.hpp>
#include <REA/Context/Global.hpp>
#include <SplitEngine/Application.hpp>
#include <SplitEngine/Contexts.hpp>


namespace REA::System
{
	MainMenu::MainMenu(std::vector<PixelGridBuilder::PixelCreateInfo>& pixelCreateInfos):
		_pixelCreateInfos(pixelCreateInfos) {}

	void MainMenu::ExecuteArchetypes(std::vector<ECS::Archetype*>& archetypes, ECS::ContextProvider& contextProvider, uint8_t stage)
	{
		EngineContext* engineContext = contextProvider.GetContext<EngineContext>();
		ECS::Registry& ecs           = engineContext->Application->GetECSRegistry();

		if (ecs.GetPrimaryGroup() == Level::MainMenu)
		{
			ImGuiHelper::CenterNextWindow();

			float oldSize = ImGui::GetFont()->Scale;
			ImGui::GetFont()->Scale *= 2.0f;
			ImGui::PushFont(ImGui::GetFont());

			ImGui::Begin("Main Menu", &_open, _flags);

			if (ImGuiHelper::ButtonCenteredOnLine("Start Sandbox"))
			{
				ecs.SetPrimaryGroup(Level::Sandbox);

				PixelGridBuilder     pixelGridBuilder{};
				Component::PixelGrid pixelGrid = pixelGridBuilder.WithSize({ 8, 8 }, { Constants::CHUNKS_X, Constants::CHUNKS_Y }).WithPixelData(_pixelCreateInfos).Build();
				pixelGrid.CameraEntityID       = contextProvider.GetContext<Context::Global>()->CameraEntityID;
				pixelGrid.InitialClear         = true;

				Component::Camera& camera = ecs.GetComponent<Component::Camera>(pixelGrid.CameraEntityID);

				camera.TargetPosition = glm::vec2(static_cast<float>(pixelGrid.WorldWidth) * 0.5f), (static_cast<float>(pixelGrid.WorldHeight) * 0.5f);

				ecs.CreateEntity<Component::Transform, Component::PixelGrid, Component::PixelGridRenderer>({}, std::move(pixelGrid), {});
			}

			if (ImGuiHelper::ButtonCenteredOnLine("Start Explorer"))
			{
				ecs.SetPrimaryGroup(Level::Explorer);

				PixelGridBuilder     pixelGridBuilder{};
				Component::PixelGrid pixelGrid = pixelGridBuilder.WithSize({ 64, 64 }, { Constants::CHUNKS_X, Constants::CHUNKS_Y }).WithPixelData(_pixelCreateInfos).Build();
				pixelGrid.CameraEntityID       = contextProvider.GetContext<Context::Global>()->CameraEntityID;
				pixelGrid.InitialClear         = false;

				Component::Camera& camera = ecs.GetComponent<Component::Camera>(pixelGrid.CameraEntityID);

				AssetDatabase& assetDatabase = engineContext->Application->GetAssetDatabase();

				b2PolygonShape boxShape{};
				boxShape.SetAsBox(1.0f, 1.0f);

				AssetHandle<Sprite> sprite = assetDatabase.GetAsset<Sprite>(Asset::Sprite::Rea_Idle_R);
				LOG("sprite {0}", sprite.GetID());

				uint64_t playerEntity = ecs.CreateEntity<Component::Transform, Component::Collider, Component::Player, Component::SpriteRenderer>({ { 20.0f, 20.0f, -10.0f } },
					{ assetDatabase.GetAsset<PhysicsMaterial>(Asset::PhysicsMaterial::Defaut), b2BodyType::b2_kinematicBody, { boxShape } },
					{},
					{ assetDatabase.GetAsset<Sprite>(Asset::Sprite::Rea_Idle_R), 1.0f, 0 });

				camera.TargetEntity = playerEntity;

				std::vector<Pixel::State> world = std::vector<Pixel::State>(pixelGrid.WorldWidth * pixelGrid.WorldHeight, pixelGrid.PixelLookup[PixelType::Air].PixelState);
				WorldGenerator::GenerationSettings generationSettings{};
				WorldGenerator::GenerateWorld(world, pixelGrid, generationSettings);

				// Split up world into chunks
				for (int chunkY = 0; chunkY < pixelGrid.WorldChunksY; ++chunkY)
				{
					for (int chunkX = 0; chunkX < pixelGrid.WorldChunksX; ++chunkX)
					{
						std::vector<Pixel::State>& chunk = pixelGrid.World[chunkY * pixelGrid.WorldChunksX + chunkX];

						for (int y = 0; y < Constants::CHUNK_SIZE; ++y)
						{
							for (int x = 0; x < Constants::CHUNK_SIZE; ++x)
							{
								int worldX = chunkX * Constants::CHUNK_SIZE + x;
								int worldY = chunkY * Constants::CHUNK_SIZE + y;

								if (worldX < pixelGrid.WorldWidth && worldY < pixelGrid.WorldHeight)
								{
									int worldIndex    = worldY * pixelGrid.WorldWidth + worldX;
									int chunkIndex    = y * Constants::CHUNK_SIZE + x;
									chunk[chunkIndex] = world[worldIndex];
								}
							}
						}
					}
				}

				uint64_t pixelGridEntityID = ecs.CreateEntity<Component::Transform, Component::PixelGrid, Component::PixelGridRenderer>({}, std::move(pixelGrid), {});

				ecs.GetComponent<Component::Player>(playerEntity).PixelGridEntityID = pixelGridEntityID;
			}

			if (ImGuiHelper::ButtonCenteredOnLine("Quit")) { engineContext->Application->Quit(); }

			ImGui::GetFont()->Scale = oldSize;
			ImGui::PopFont();
			ImGui::End();
		}
	}
}
