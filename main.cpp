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
#include "REA/Component/PixelGridRenderer.hpp"
#include "REA/Context/ImGui.hpp"
#include "REA/System/GameOfLifeSimulation.hpp"
#include "REA/System/ImGuiManager.hpp"

using namespace SplitEngine;
using namespace REA;

int main()
{
	// Setup Pixels Types
	float                                          airTemperature = 30;
	std::vector<PixelGridBuilder::PixelCreateInfo> pixelLookup    = {
		{
			.ID = PixelType::Air,
			.Name = "Air",
			.Color = Color(0x5890FFFF),
			.Data = { .Flags = BitSet<uint32_t>(Pixel::Gravity), .Density = 10, .SpreadingFactor = 4, .TemperatureResistance = 0.3f, .BaseTemperature = airTemperature, },
		},
		{
			.ID = PixelType::Sand,
			.Name = "Sand",
			.Color = Color(0x9F944BFF),
			.Data =
			{ .Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::Gravity), .Density = 14, .SpreadingFactor = 0, .TemperatureResistance = 1.0f, .BaseTemperature = airTemperature, }
		},
		{
			.ID = PixelType::Water,
			.Name = "Water",
			.Color = Color(0x84BCFFFF),
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Gravity | Pixel::Electricity),
				.Density = 12,
				.SpreadingFactor = 8,
				.TemperatureResistance = 0.6f,
				.BaseTemperature = airTemperature,
				.HighTemperatureLimit = 100,
				.HighTemperatureLimitPixelID = PixelType::Steam,
				.ChargeAbsorbtionChance = 0.75f,
			}
		},
		{
			.ID = PixelType::Wood,
			.Name = "Wood",
			.Color = Color(0x775937FF),
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::Electricity | Pixel::ElectricityReceiver),
				.Density = 0,
				.SpreadingFactor = 0,
				.TemperatureResistance = 1.0f,
				.BaseTemperature = airTemperature,
				.HighTemperatureLimit = 600,
				.HighTemperatureLimitPixelID = PixelType::Fire,
			}
		},
		{
			.ID = PixelType::Steam,
			.Name = "Steam",
			.Color = Color(0xAAAAAAFF),
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 8,
				.SpreadingFactor = 5,
				.TemperatureResistance = 0.1f,
				.BaseTemperature = 100,
				.LowerTemperatureLimit = 95,
				.LowerTemperatureLimitPixelID = PixelType::Water,
			}
		},
		{
			.ID = PixelType::Oil,
			.Name = "Oil",
			.Color = Color(0x333333FF),
			.Data =
			{ .Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::Gravity), .Density = 16, .SpreadingFactor = 2, .TemperatureResistance = 0, .BaseTemperature = airTemperature, }
		},
		{
			.ID = PixelType::Lava,
			.Name = "Lava",
			.Color = Color(0xFF8619FF),
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 20,
				.SpreadingFactor = 2,
				.TemperatureResistance = 0.01f,
				.BaseTemperature = 1600,
				.LowerTemperatureLimit = 80,
				.LowerTemperatureLimitPixelID = PixelType::Stone,
			}
		},
		{
			.ID = PixelType::Stone,
			.Name = "Stone",
			.Color = Color(0x465466FF),
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Gravity | Pixel::Solid),
				.Density = 18,
				.SpreadingFactor = 0,
				.TemperatureResistance = 0.01f,
				.BaseTemperature = airTemperature,
				.HighTemperatureLimit = 180,
				.HighTemperatureLimitPixelID = PixelType::Lava,
			}
		},
		{
			.ID = PixelType::Fire,
			.Name = "Fire",
			.Color = Color(0xFF8331FF),
			.Data = {
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 5,
				.SpreadingFactor = 1,
				.TemperatureResistance = 0.1f,
				.BaseTemperature = 1200,
				.LowerTemperatureLimit = 60,
				.LowerTemperatureLimitPixelID = PixelType::Smoke,
				.TemperatureConversion = 1.0f,
			}
		},
		{
			.ID = PixelType::Smoke,
			.Name = "Smoke",
			.Color = Color(0x525252FF),
			.Data = {
				.Flags = BitSet<uint32_t>(Pixel::Gravity),
				.Density = 3,
				.SpreadingFactor = 5,
				.TemperatureResistance = 1.0f,
				.BaseTemperature = 50,
				.LowerTemperatureLimit = 35,
				.LowerTemperatureLimitPixelID = PixelType::Air,
			}
		},
		{
			.ID = PixelType::Spark,
			.Name = "Spark",
			.Color = Color(0xFFFA66FF),
			.Data = { .Flags = BitSet<uint32_t>(Pixel::Gravity | Pixel::Electricity | Pixel::ElectricityEmitter), .Density = 3, .SpreadingFactor = 5, .TemperatureResistance = 1.0f, .BaseCharge = 255, }
		},
		{
			.ID = PixelType::Iron,
			.Name = "Iron",
			.Color = Color(0x313642FF),
			.Data = { .Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::Electricity), .Density = 100, .BaseCharge = 0, }
		},
	};

	Application application = Application({ {}, {}, { false, 5688 } });

	application.GetWindow().SetSize(1000, 1000);

	Input::RegisterAxis2D(InputAction::Move, { KeyCode::A, KeyCode::D }, { KeyCode::S, KeyCode::W });
	Input::RegisterButtonAction(InputAction::Fire, KeyCode::MOUSE_LEFT);

	AssetDatabase& assetDatabase = application.GetAssetDatabase();


	AssetHandle<Rendering::Shader> spriteShader = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Sprite,
	                                                                                           { { "res/shaders/debug/debug.vert", "res/shaders/debug/debug.frag" } });
	AssetHandle<Rendering::Material> spriteMaterial = assetDatabase.CreateAsset<Rendering::Material>(Material::Sprite, { spriteShader });

	AssetHandle<Rendering::Shader> pixelGridShader = assetDatabase.CreateAsset<Rendering::Shader>(Shader::PixelGrid,
	                                                                                              {
		                                                                                              {
			                                                                                              "res/shaders/PixelGrid/PixelGrid.vert",
			                                                                                              "res/shaders/PixelGrid/PixelGrid.frag"
		                                                                                              }
	                                                                                              });
	AssetHandle<Rendering::Material> pixelGridMaterial = assetDatabase.CreateAsset<Rendering::Material>(Material::PixelGrid, { pixelGridShader });

	// Create texture page and sprite assets
	Tools::ImagePacker texturePacker = Tools::ImagePacker();

	uint64_t floppaPackerID     = texturePacker.AddImage("res/textures/Floppa.png");
	uint64_t blueBulletPackerID = texturePacker.AddRelatedImages(Tools::ImageSlicer::Slice("res/textures/BlueBullet.png", { 3 }));

	Tools::ImagePacker::PackingData packingData = texturePacker.Pack(2048);

	AssetHandle<SpriteTexture> floppaSprite     = assetDatabase.CreateAsset<SpriteTexture>(Sprite::Floppa, { floppaPackerID, packingData });
	AssetHandle<SpriteTexture> blueBulletSprite = assetDatabase.CreateAsset<SpriteTexture>(Sprite::BlueBullet, { blueBulletPackerID, packingData });

	AssetHandle<Rendering::Shader> pixelGridComputeIdle = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Comp_PixelGrid_Idle,
	                                                                                                   { { "res/shaders/PixelGridComputeIdle/PixelGridComputeIdle.comp" } });

	AssetHandle<Rendering::Shader> pixelGridComputeFall = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Comp_PixelGrid_Fall,
	                                                                                                   { { "res/shaders/PixelGridComputeFalling/PixelGridComputeFalling.comp" } });

	AssetHandle<Rendering::Shader> pixelGridComputeAccumulate = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Comp_PixelGrid_Accumulate,
	                                                                                                         {
		                                                                                                         {
			                                                                                                         "res/shaders/PixelGridComputeAccumulate/PixelGridComputeAccumulate.comp"
		                                                                                                         }
	                                                                                                         });

	ECS::Registry& ecs = application.GetECSRegistry();

	ecs.RegisterComponent<Component::Transform>();
	ecs.RegisterComponent<Component::SpriteRenderer>();
	ecs.RegisterComponent<Component::Physics>();
	ecs.RegisterComponent<Component::AudioSource>();
	ecs.RegisterComponent<Component::Player>();
	ecs.RegisterComponent<Component::Camera>();
	ecs.RegisterComponent<Component::PixelGrid>();
	ecs.RegisterComponent<Component::PixelGridRenderer>();

	ecs.RegisterContext<Context::ImGui>({});

	ecs.AddSystem<System::Debug>(Stage::Gameplay, -1);
	ecs.AddSystem<System::ImGuiManager>(EngineStage::EndRendering, EngineStageOrder::EndRendering_RenderingSystem - 1);

	ecs.AddSystem<System::PixelGridDrawing>(Stage::GridComputeHalted, 0, 1, PixelType::Air);
	ecs.AddSystem<System::Camera>(Stage::Gameplay, 1);
	ecs.AddSystem<System::AudioSourcePlayer>(Stage::Gameplay, 999);

	System::PixelGridSimulation::SimulationShaders simulationShaders = {
		.IdleSimulation = pixelGridComputeIdle,
		.FallingSimulation = pixelGridComputeFall,
		.AccumulateSimulation = pixelGridComputeAccumulate
	};

	ecs.AddSystem<System::PixelGridSimulation>({ { Stage::GridComputeEnd, 1000 }, { Stage::GridComputeBegin, 1000 }, }, simulationShaders);
	//ecs.AddSystem<System::GameOfLifeSimulation>(ECS::Stage::Gameplay, 999);


	ecs.AddSystem<System::RenderingPreparation>(Stage::Rendering, 0);
	ecs.AddSystem<System::PixelGridRenderer>(Stage::Rendering, 1, pixelGridMaterial);
	//ecs.AddSystem<System::SpriteRenderer>(ECS::Stage::Rendering, 2, spriteMaterial, packingData);

	// Create entities
	uint64_t playerEntity = ecs.CreateEntity<Component::Transform, Component::Physics, Component::Player, Component::SpriteRenderer>({}, {}, {}, { floppaSprite, 1.0f });

	ecs.CreateEntity<Component::Transform, Component::Camera>({}, { playerEntity });


	PixelGridBuilder     pixelGridBuilder{};
	Component::PixelGrid pixelGrid = pixelGridBuilder.WithSize({ 1000, 1000 }).WithPixelData(std::move(pixelLookup)).Build();

	ecs.CreateEntity<Component::PixelGrid, Component::PixelGridRenderer>(std::move(pixelGrid), {});

	// Run Game
	application.Run();

	return 0;
}
