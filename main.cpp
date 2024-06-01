#include <imgui.h>
#include <imgui_internal.h>

#include "SplitEngine/Application.hpp"
#include "SplitEngine/AssetDatabase.hpp"
#include "SplitEngine/Input.hpp"
#include "SplitEngine/Debug/Performance.hpp"
#include "SplitEngine/ECS/System.hpp"
#include "SplitEngine/Tools/ImagePacker.hpp"
#include "SplitEngine/Tools/ImageSlicer.hpp"
#include <SplitEngine/IO/ImageLoader.hpp>
#include <SplitEngine/Rendering/Texture2D.hpp>


#include <glm/gtc/random.hpp>

#include "REA/Assets.hpp"
#include "REA/Component/AudioSource.hpp"
#include "REA/Component/Camera.hpp"
#include "REA/Component/Physics.hpp"
#include "REA/Component/Player.hpp"
#include "REA/Component/SpriteRenderer.hpp"
#include "REA/Component/Transform.hpp"
#include "REA/System/AudioSourcePlayer.hpp"
#include "REA/System/Camera.hpp"
#include "REA/System/Debug.hpp"
#include "REA/System/Physics.hpp"
#include "REA/System/PixelGridRenderer.hpp"
#include "REA/System/PixelGridSimulation.hpp"
#include "REA/System/PlayerController.hpp"
#include "REA/System/RenderingPreparation.hpp"
#include "REA/System/SpriteRenderer.hpp"
#include "include/REA/System/PixelGridDrawing.hpp"
#include "REA/PixelGridBuilder.hpp"
#include "REA/PixelType.hpp"
#include "REA/Stage.hpp"
#include "REA/Context/ImGui.hpp"
#include "REA/System/GameOfLifeSimulation.hpp"
#include "REA/System/ImGuiManager.hpp"

using namespace SplitEngine;
using namespace REA;

int main()
{
	// Setup Pixels Types
	std::vector<PixelGridBuilder::PixelCreateInfo> pixelLookup = {
		{
			.ID = PixelType::Air,
			.Name = "Air",
			.Color = Color(0x5890FFFF),
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 10,
				.SpreadingFactor = 4,
				.TemperatureResistance = 0,
			},
		},
		{
			.ID = PixelType::Sand,
			.Name = "Sand",
			.Color = Color(0x9F944BFF),
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 14,
				.SpreadingFactor = 0,
				.TemperatureResistance = 0,
			}
		},
		{
			.ID = PixelType::Water,
			.Name = "Water",
			.Color = Color(0x84BCFFFF),
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 12,
				.SpreadingFactor = 8,
				.TemperatureResistance = 0.1f,
				.HighTemperatureLimit = 128,
				.HighTemperatureLimitPixelID = PixelType::Smoke,
			}
		},
		{
			.ID = PixelType::Wood,
			.Name = "Wood",
			.Color = Color(0x775937FF),
			.Data = {
				.Flags = BitSet<uint32_t>(Pixel::Solid),
				.Density = 15,
				.SpreadingFactor = 0,
				.TemperatureResistance = 0,

			}
		},
		{
			.ID = PixelType::Smoke,
			.Name = "Smoke",
			.Color = Color(0xAAAAAAFF),
			.Data = {
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 8,
				.SpreadingFactor = 2,
				.TemperatureResistance = 0,
				.BaseTemperature = 129,
				.LowerTemperatureLimit = 128,
				.LowerTemperatureLimitPixelID = PixelType::Water,
			}
		},
		{
			.ID = PixelType::Oil,
			.Name = "Oil",
			.Color = Color(0x333333FF),
			.Data = {
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 11,
				.SpreadingFactor = 2,
				.TemperatureResistance = 0,
				.BaseTemperature = 0,
			}
		},
		{
			.ID = PixelType::Lava,
			.Name = "Lava",
			.Color = Color(0xFF8619FF),
			.Data = {
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 11,
				.SpreadingFactor = 2,
				.TemperatureResistance = 0,
				.BaseTemperature = 10,
			}
		},
	};

	Application application = Application({ {}, { .UseVulkanValidationLayers = false } });

	application.GetWindow().SetSize(1000, 1000);

	Input::RegisterAxis2D(InputAction::Move, { KeyCode::A, KeyCode::D }, { KeyCode::S, KeyCode::W });
	Input::RegisterButtonAction(InputAction::Fire, KeyCode::MOUSE_LEFT);

	AssetDatabase& assetDatabase = application.GetAssetDatabase();

	AssetHandle<Rendering::Shader>   spriteShader   = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Sprite, { "res/shaders/debug" });
	AssetHandle<Rendering::Material> spriteMaterial = assetDatabase.CreateAsset<Rendering::Material>(Material::Sprite, { spriteShader });

	AssetHandle<Rendering::Shader>   pixelGridShader   = assetDatabase.CreateAsset<Rendering::Shader>(Shader::PixelGrid, { "res/shaders/PixelGrid" });
	AssetHandle<Rendering::Material> pixelGridMaterial = assetDatabase.CreateAsset<Rendering::Material>(Material::PixelGrid, { pixelGridShader });

	// Create texture page and sprite assets
	Tools::ImagePacker texturePacker = Tools::ImagePacker();

	uint64_t floppaPackerID     = texturePacker.AddImage("res/textures/Floppa.png");
	uint64_t blueBulletPackerID = texturePacker.AddRelatedImages(Tools::ImageSlicer::Slice("res/textures/BlueBullet.png", { 3 }));

	Tools::ImagePacker::PackingData packingData = texturePacker.Pack(2048);

	AssetHandle<SpriteTexture> floppaSprite     = assetDatabase.CreateAsset<SpriteTexture>(Sprite::Floppa, { floppaPackerID, packingData });
	AssetHandle<SpriteTexture> blueBulletSprite = assetDatabase.CreateAsset<SpriteTexture>(Sprite::BlueBullet, { blueBulletPackerID, packingData });

	AssetHandle<Rendering::Shader> pixelGridComputeIdle = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Comp_PixelGrid_Idle, { "res/shaders/PixelGridComputeIdle" });
	AssetHandle<Rendering::Shader> pixelGridComputeFall = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Comp_PixelGrid_Fall, { "res/shaders/PixelGridComputeFalling" });
	AssetHandle<Rendering::Shader> pixelGridComputeAccumulate = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Comp_PixelGrid_Accumulate,
	                                                                                                         { "res/shaders/PixelGridComputeAccumulate" });

	ECS::Registry& ecs = application.GetECSRegistry();

	ecs.RegisterComponent<Component::Transform>();
	ecs.RegisterComponent<Component::SpriteRenderer>();
	ecs.RegisterComponent<Component::Physics>();
	ecs.RegisterComponent<Component::AudioSource>();
	ecs.RegisterComponent<Component::Player>();
	ecs.RegisterComponent<Component::Camera>();
	ecs.RegisterComponent<Component::PixelGrid>();

	ecs.RegisterContext<Context::ImGui>({});

	ecs.AddSystem<System::Debug>(Stage::Gameplay, -1);
	ecs.AddSystem<System::ImGuiManager>(EngineStage::EndRendering, EngineStageOrder::EndRendering_RenderingSystem - 1);

	ecs.AddSystem<System::PixelGridDrawing>(Stage::Gameplay, 0, 1);
	ecs.AddSystem<System::Camera>(Stage::Gameplay, 1);
	ecs.AddSystem<System::AudioSourcePlayer>(Stage::Gameplay, 999);

	System::PixelGridSimulation::SimulationShaders simulationShaders = {
		.IdleSimulation = pixelGridComputeIdle,
		.FallingSimulation = pixelGridComputeFall,
		.AccumulateSimulation = pixelGridComputeAccumulate
	};
	ecs.AddSystem<System::PixelGridSimulation>(Stage::Gameplay, 999, simulationShaders, PixelType::Air);
	//ecs.AddSystem<System::GameOfLifeSimulation>(ECS::Stage::Gameplay, 999);

	ecs.AddSystem<System::RenderingPreparation>(Stage::Rendering, 0);
	ecs.AddSystem<System::PixelGridRenderer>(Stage::Rendering, 1, pixelGridMaterial);
	//ecs.AddSystem<System::SpriteRenderer>(ECS::Stage::Rendering, 2, spriteMaterial, packingData);

	// Create entities
	uint64_t playerEntity = ecs.CreateEntity<Component::Transform, Component::Physics, Component::Player, Component::SpriteRenderer>({}, {}, {}, { floppaSprite, 1.0f });

	ecs.CreateEntity<Component::Transform, Component::Camera>({}, { playerEntity });


	PixelGridBuilder     pixelGridBuilder{};
	Component::PixelGrid pixelGrid = pixelGridBuilder.WithSize({ 1000, 1000 }).WithPixelData(std::move(pixelLookup)).Build();

	uint64_t pixelGridEntity = ecs.CreateEntity<Component::PixelGrid>(std::move(pixelGrid));

	// Run Game
	application.Run();

	return 0;
}
