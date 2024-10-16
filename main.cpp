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
#include <REA/Component/SandboxController.hpp>
#include <REA/System/MainMenu.hpp>
#include <REA/System/PauseMenu.hpp>
#include <REA/System/PlayerAnimation.hpp>
#include <REA/System/SandboxController.hpp>

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
#include "REA/PhysicsMaterial.hpp"
#include "REA/PixelGridBuilder.hpp"
#include "REA/PixelType.hpp"
#include "REA/Stage.hpp"
#include "REA/Component/PixelGridRenderer.hpp"
#include "REA/Component/PixelGridRigidBody.hpp"
#include "REA/Context/ImGui.hpp"
#include "REA/Context/Global.hpp"
#include "REA/System/ImGuiManager.hpp"
#include "REA/System/PhysicsDebugRenderer.hpp"

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
			.Color = Color(0x183675FF),
			.Category = Pixel::Category::Liquid,
			.Data = { .Flags = BitSet<uint32_t>(Pixel::Gravity), .Density = 10, .SpreadingFactor = 4, .TemperatureResistance = 0.3f, .BaseTemperature = airTemperature, },
		},
		{
			.ID = PixelType::Sand,
			.Name = "Sand",
			.Color = Color(0x9F944BFF),
			.Category = Pixel::Category::Sand,
			.Data =
			{ .Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::Gravity), .Density = 14, .SpreadingFactor = 0, .TemperatureResistance = 1.0f, .BaseTemperature = airTemperature, }
		},
		{
			.ID = PixelType::Water,
			.Name = "Water",
			.Color = Color(0x84BCFFFF),
			.Category = Pixel::Category::Liquid,
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Gravity | Pixel::Conductive),
				.Density = 12,
				.SpreadingFactor = 16,
				.TemperatureResistance = 0.6f,
				.BaseTemperature = airTemperature,
				.HighTemperatureLimit = 100,
				.HighTemperatureLimitPixelID = PixelType::Steam,
			}
		},
		{
			.ID = PixelType::Wood,
			.Name = "Wood",
			.Color = Color(0x775937FF),
			.Category = Pixel::Category::Static,
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::Conductive),
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
			.Category = Pixel::Category::Liquid,
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
			.Category = Pixel::Category::Liquid,
			.Data = { .Flags = BitSet<uint32_t>(Pixel::Gravity), .Density = 16, .SpreadingFactor = 2, .TemperatureResistance = 0, .BaseTemperature = airTemperature, }
		},
		{
			.ID = PixelType::Lava,
			.Name = "Lava",
			.Color = Color(0xFF8619FF),
			.Category = Pixel::Category::Liquid,
			.Data = { .Flags = BitSet<uint32_t>(Pixel::Gravity), .Density = 20, .SpreadingFactor = 2, .TemperatureResistance = 0.00f, .BaseTemperature = 1600, }
		},
		{
			.ID = PixelType::Stone,
			.Name = "Stone",
			.Color = Color(0x465466FF),
			.Category = Pixel::Category::Static,
			.Data = { .Flags = BitSet<uint32_t>(Pixel::Solid), .Density = 18, .TemperatureResistance = 0.01f, }
		},
		{
			.ID = PixelType::Gravel,
			.Name = "Gravel",
			.Color = Color(0x708D91FF),
			.Category = Pixel::Category::Sand,
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
			.Category = Pixel::Category::Liquid,
			.Data =
			{
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
			.Category = Pixel::Category::Liquid,
			.Data =
			{
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
			.Color = Color(0xC09D55FF),
			.Category = Pixel::Category::Electronic,
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Gravity | Pixel::Conductive | Pixel::ElectricityEmitter),
				.Density = 3,
				.SpreadingFactor = 5,
				.TemperatureResistance = 1.0f,
				.BaseCharge = 255,
			}
		},
		{
			.ID = PixelType::Iron,
			.Name = "Iron",
			.Color = Color(0x313642FF),
			.Category = Pixel::Category::Static,
			.Data = { .Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::Conductive), .Density = 100, .TemperatureResistance = 1.0f, .BaseCharge = 0, .AcidityResistance = 200.0f }
		},
		{ .ID = PixelType::Dirt, .Name = "Dirt", .Color = Color(0x916B4AFF), .Data = Pixel::Data{ .Flags = BitSet<uint32_t>(Pixel::Solid), .Density = 100 } },
		{
			.ID = PixelType::Grass,
			.Name = "Grass",
			.Color = Color(0x2E922EFF),
			.Category = Pixel::Category::Static,
			.Data = Pixel::Data
			{
				.Flags = BitSet<uint32_t>(Pixel::Solid),
				.Density = 100,
				.TemperatureResistance = 1.0f,
				.BaseTemperature = airTemperature,
				.HighTemperatureLimit = 600,
				.HighTemperatureLimitPixelID = PixelType::Fire,
			}
		},
		{
			.ID = PixelType::Leaf,
			.Name = "Leaf",
			.Color = Color(0x509229FF),
			.Category = Pixel::Category::Static,
			.Data = Pixel::Data
			{
				.Flags = BitSet<uint32_t>(Pixel::Solid),
				.Density = 100,
				.TemperatureResistance = 1.0f,
				.BaseTemperature = airTemperature,
				.HighTemperatureLimit = 300,
				.HighTemperatureLimitPixelID = PixelType::Fire,
			}
		},
		{
			.ID = PixelType::Acid,
			.Name = "Acid",
			.Color = Color(0x28F600FF),
			.Category = Pixel::Category::Liquid,
			.Data = Pixel::Data{ .Flags = BitSet<uint32_t>(Pixel::Gravity), .Density = 13, .SpreadingFactor = 6, .BaseTemperature = airTemperature, .Acidity = 100.0f, }
		},
		{
			.ID = PixelType::Wood_Rigidbody,
			.Name = "Wood Rigidbody",
			.Color = Color(0x775937FF),
			.Category = Pixel::Category::Rigidbody,
			.Data =
			{
				.Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::Connected | Pixel::Conductive),
				.Density = 0,
				.SpreadingFactor = 0,
				.TemperatureResistance = 1.0f,
				.BaseTemperature = airTemperature,
				.HighTemperatureLimit = 600,
				.HighTemperatureLimitPixelID = PixelType::Fire,
			},
		},
		{
			.ID = PixelType::Pulser,
			.Name = "Pulser",
			.Color = Color(0x8735A0FF),
			.Category = Pixel::Category::Electronic,
			.Data = Pixel::Data{ .Flags = BitSet<uint32_t>(Pixel::Solid), .BaseTemperature = airTemperature, .Pulser = true, }
		},
		{
			.ID = PixelType::LED_Red,
			.Name = "LED Red",
			.Color = Color(0xFFB3ACFF),
			.Category = Pixel::Category::Electronic,
			.Data = Pixel::Data
			{
				.Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::ElectricityReceiver),
				.BaseTemperature = airTemperature,
				.HighChargeLimit = 127,
				.HighChargeLimitPixelID = PixelType::LED_Red_On
			}
		},
		{
			.ID = PixelType::LED_Red_On,
			.Name = "LED Red On",
			.Color = Color(0xFF3233FF),
			.Category = Pixel::Category::Electronic,
			.Data = Pixel::Data
			{
				.Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::ElectricityReceiver),
				.BaseTemperature = airTemperature,
				.LowerChargeLimit = 127,
				.LowerChargeLimitPixelID = PixelType::LED_Red
			}
		},
		{
			.ID = PixelType::LED_Green,
			.Name = "LED Green",
			.Color = Color(0xB7FFA6FF),
			.Category = Pixel::Category::Electronic,
			.Data = Pixel::Data
			{
				.Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::ElectricityReceiver),
				.BaseTemperature = airTemperature,
				.HighChargeLimit = 127,
				.HighChargeLimitPixelID = PixelType::LED_Green_On
			}
		},
		{
			.ID = PixelType::LED_Green_On,
			.Name = "LED Green On",
			.Color = Color(0x58FF59FF),
			.Category = Pixel::Category::Electronic,
			.Data = Pixel::Data
			{
				.Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::ElectricityReceiver),
				.BaseTemperature = airTemperature,
				.LowerChargeLimit = 127,
				.LowerChargeLimitPixelID = PixelType::LED_Green
			}
		},
		{
			.ID = PixelType::LED_Blue,
			.Name = "LED Blue",
			.Color = Color(0x89B5FFFF),
			.Category = Pixel::Category::Electronic,
			.Data = Pixel::Data{
				.Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::ElectricityReceiver),
				.BaseTemperature = airTemperature,
				.HighChargeLimit = 127,
				.HighChargeLimitPixelID = PixelType::LED_Blue_On
			}
		},
		{
			.ID = PixelType::LED_Blue_On,
			.Name = "LED Blue On",
			.Color = Color(0x2533FFFF),
			.Category = Pixel::Category::Electronic,
			.Data = Pixel::Data{
				.Flags = BitSet<uint32_t>(Pixel::Solid | Pixel::ElectricityReceiver),
				.BaseTemperature = airTemperature,
				.LowerChargeLimit = 127,
				.LowerChargeLimitPixelID = PixelType::LED_Blue
			}
		},
	};

	Application application = Application({
		                                      {},
		                                      { .RootExecutionStages = { Stage::PrePhysicsStep, Stage::Physics } },
		                                      {},
		                                      { .UseVulkanValidationLayers = false, .ViewportStyle = Rendering::Vulkan::ViewportStyle::Flipped, .ClearColor = Color(0x000000FF) }
	                                      });

	application.GetWindow().SetSize(1024, 1024);

	Input::RegisterAxis2D(InputAction::Fly, { KeyCode::A, KeyCode::D }, { KeyCode::S, KeyCode::W });
	Input::RegisterAxis(InputAction::Move, { KeyCode::A, KeyCode::D });
	Input::RegisterButtonAction(InputAction::Fire, KeyCode::MOUSE_LEFT);
	Input::RegisterButtonAction(InputAction::Jump, KeyCode::SPACE);

	AssetDatabase& assetDatabase = application.GetAssetDatabase();

	AssetHandle<Rendering::Shader> spriteShader = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Sprite,
	                                                                                           { { "res/shaders/Sprite/Sprite.vert", "res/shaders/Sprite/Sprite.frag" } });

	AssetHandle<Rendering::Material> spriteMaterial = assetDatabase.CreateAsset<Rendering::Material>(Asset::Material::Sprite, { spriteShader });


	Rendering::Vulkan::PipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.AssemblyStateCreateInfo.topology               = vk::PrimitiveTopology::eTriangleFan;
	pipelineCreateInfo.AssemblyStateCreateInfo.primitiveRestartEnable = vk::True;
	pipelineCreateInfo.RasterizationStateCreateInfo.polygonMode       = vk::PolygonMode::eLine;
	AssetHandle<Rendering::Shader> physicsDebugShader                 = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::PhysicsDebug,
	                                                                                                                 {
		                                                                                                                 {
			                                                                                                                 "res/shaders/PhysicsDebug/PhysicsDebug.vert",
			                                                                                                                 "res/shaders/PhysicsDebug/PhysicsDebug.frag"
		                                                                                                                 },
		                                                                                                                 pipelineCreateInfo
	                                                                                                                 });

	pipelineCreateInfo.AssemblyStateCreateInfo.primitiveRestartEnable = vk::False;
	pipelineCreateInfo.AssemblyStateCreateInfo.topology               = vk::PrimitiveTopology::eLineList;
	AssetHandle<Rendering::Shader> marchingSquareDebugShader          = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::MarchingSquare,
	                                                                                                                 {
		                                                                                                                 {
			                                                                                                                 "res/shaders/PhysicsDebug/PhysicsDebug.vert",
			                                                                                                                 "res/shaders/PhysicsDebug/PhysicsDebug.frag"
		                                                                                                                 },
		                                                                                                                 pipelineCreateInfo
	                                                                                                                 });

	AssetHandle<Rendering::Material> physicsDebugMaterial = assetDatabase.CreateAsset<Rendering::Material>(Asset::Material::PhysicsDebug, { physicsDebugShader });

	AssetHandle<Rendering::Material> marchingSqaureDebugMaterial = assetDatabase.CreateAsset<Rendering::Material>(Asset::Material::MarchingSquare, { marchingSquareDebugShader });


	AssetHandle<Rendering::Shader> pixelGridShader = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::PixelGrid,
	                                                                                              {
		                                                                                              {
			                                                                                              "res/shaders/PixelGrid/PixelGrid.vert",
			                                                                                              "res/shaders/PixelGrid/PixelGrid.frag"
		                                                                                              }
	                                                                                              });
	AssetHandle<Rendering::Material> pixelGridMaterial = assetDatabase.CreateAsset<Rendering::Material>(Asset::Material::PixelGrid, { pixelGridShader });

	// Create texture page and sprite assets
	Tools::ImagePacker texturePacker = Tools::ImagePacker();

	uint64_t floppaPackerID     = texturePacker.AddImage("res/textures/Floppa.png");
	uint64_t blueBulletPackerID = texturePacker.AddRelatedImages(Tools::ImageSlicer::Slice("res/textures/BlueBullet.png", { 3 }));
	uint64_t reaIdleRPackerID   = texturePacker.AddImage("res/textures/Rea_Idle_R.png");
	uint64_t reaIdleLPackerID   = texturePacker.AddImage("res/textures/Rea_Idle_L.png");
	uint64_t reaWalkRPackerID   = texturePacker.AddRelatedImages(Tools::ImageSlicer::Slice("res/textures/Rea_Walk_R.png", { 10 }));
	uint64_t reaWalkLPackerID   = texturePacker.AddRelatedImages(Tools::ImageSlicer::Slice("res/textures/Rea_Walk_L.png", { 10 }));

	Tools::ImagePacker::PackingData packingData = texturePacker.Pack(2048);

	AssetHandle<Sprite> floppaSprite     = assetDatabase.CreateAsset<Sprite>(Asset::Sprite::Floppa, { floppaPackerID, packingData });
	AssetHandle<Sprite> blueBulletSprite = assetDatabase.CreateAsset<Sprite>(Asset::Sprite::BlueBullet, { blueBulletPackerID, packingData });
	AssetHandle<Sprite> reaIdleRSprite   = assetDatabase.CreateAsset<Sprite>(Asset::Sprite::Rea_Idle_R, { reaIdleRPackerID, packingData });
	AssetHandle<Sprite> reaWalkRSprite   = assetDatabase.CreateAsset<Sprite>(Asset::Sprite::Rea_Walk_R, { reaWalkRPackerID, packingData });
	AssetHandle<Sprite> reaIdleLSprite   = assetDatabase.CreateAsset<Sprite>(Asset::Sprite::Rea_Idle_L, { reaIdleLPackerID, packingData });
	AssetHandle<Sprite> reaWalkLSprite   = assetDatabase.CreateAsset<Sprite>(Asset::Sprite::Rea_Walk_L, { reaWalkLPackerID, packingData });

	auto pixelGridComputeIdle = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_PixelGrid_Idle,
	                                                                         { { "res/shaders/PixelGridComputeIdle/PixelGridComputeIdle.comp" } });

	auto pixelGridComputeRigidBody = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_PixelGrid_RigidBody,
	                                                                              { { "res/shaders/PixelGridRigidBody/PixelGridRigidBody.comp" } });

	auto pixelGridRigidBodyRemove = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_PixelGrid_RigidBody,
	                                                                             { { "res/shaders/PixelGridRigidBodyRemove/PixelGridRigidBodyRemove.comp" } });

	auto pixelGridComputeFall = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_PixelGrid_Fall,
	                                                                         { { "res/shaders/PixelGridComputeFalling/PixelGridComputeFalling.comp" } });

	auto pixelGridComputeAccumulate = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_PixelGrid_Accumulate,
	                                                                               { { "res/shaders/PixelGridComputeAccumulate/PixelGridComputeAccumulate.comp" } });

	auto pixelGridComputeParticle = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_PixelGrid_Particle,
	                                                                             { { "res/shaders/PixelGridComputeParticle/PixelGridComputeParticle.comp" } });

	auto marchingSquareShader = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_MarchingSquare, { { "res/shaders/MarchingSquare/MarchingSquare.comp" } });

	// CCL
	auto cclInitialize = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_CCLInitialize, { { "res/shaders/CCLInitialize/CCLInitialize.comp" } });
	auto cclColumn     = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_CCLColumn, { { "res/shaders/CCLColumn/CCLColumn.comp" } });
	auto cclMerge      = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_CCLMerge, { { "res/shaders/CCLMerge/CCLMerge.comp" } });
	auto cclRelabel    = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_CCLRelabel, { { "res/shaders/CCLRelabel/CCLRelabel.comp" } });
	auto cclExtract    = assetDatabase.CreateAsset<Rendering::Shader>(Asset::Shader::Comp_CCLExtract, { { "res/shaders/CCLExtract/CCLExtract.comp" } });

	AssetHandle<PhysicsMaterial> defaultPhysicsMaterial = assetDatabase.CreateAsset<PhysicsMaterial>(Asset::PhysicsMaterial::Defaut, { .Density = 1.0f });

	ECS::Registry& ecs = application.GetECSRegistry();

	ecs.SetEnableStatistics(true);

	ecs.RegisterComponent<Component::Transform>();
	ecs.RegisterComponent<Component::SpriteRenderer>();
	ecs.RegisterComponent<Component::Physics>();
	ecs.RegisterComponent<Component::AudioSource>();
	ecs.RegisterComponent<Component::Player>();
	ecs.RegisterComponent<Component::Camera>();
	ecs.RegisterComponent<Component::PixelGrid>();
	ecs.RegisterComponent<Component::PixelGridRenderer>();
	ecs.RegisterComponent<Component::Collider>();
	ecs.RegisterComponent<Component::PixelGridRigidBody>();
	ecs.RegisterComponent<Component::SandboxController>();

	ecs.RegisterContext<Context::ImGui>({});
	ecs.RegisterContext<Context::Global>({});

	// Physics System
	ECS::Registry::SystemHandle<System::Physics> physicsHandle = ecs.AddSystem<System::Physics>({ { Stage::PrePhysicsStep, 0 }, { Stage::PhysicsManagement, 0 } }, true);

	// Halted Compute Shader
	ecs.AddSystem<System::PlayerController>(Stage::GridComputeHalted, 0);
	ecs.AddSystem<System::PixelGridDrawing>(Stage::GridComputeHalted, 10, 1, PixelType::Air);

	// Gameplay
	ecs.AddSystem<System::Debug>(Stage::Gameplay, -1);
	ecs.AddSystem<System::MainMenu>(Stage::Gameplay, 1, pixelLookup);
	ecs.AddSystem<System::PauseMenu>(Stage::Gameplay, 1);
	ecs.AddSystem<System::AudioSourcePlayer>(Stage::Gameplay, 999);

	// Late gameplay
	ecs.AddSystem<System::SandboxController>(Stage::LateGameplay, 2);
	ecs.AddSystem<System::PlayerAnimation>(Stage::LateGameplay, 1);
	ecs.AddSystem<System::Camera>(Stage::LateGameplay, 1);

	// Simulation
	System::PixelGridSimulation::SimulationShaders simulationShaders = {
		.IdleSimulation = pixelGridComputeIdle,
		.RigidBodySimulation = pixelGridComputeRigidBody,
		.RigidBodyRemove = pixelGridRigidBodyRemove,
		.FallingSimulation = pixelGridComputeFall,
		.AccumulateSimulation = pixelGridComputeAccumulate,
		.PixelParticle = pixelGridComputeParticle,
		.MarchingSquareAlgorithm = marchingSquareShader,
		.CCLInitialize = cclInitialize,
		.CCLColumn = cclColumn,
		.CCLMerge = cclMerge,
		.CCLRelabel = cclRelabel,
		.CCLExtract = cclExtract,
	};

	ECS::Registry::SystemHandle<System::PixelGridSimulation> simulation = ecs.AddSystem<System::PixelGridSimulation>({
		                                                                                                                 { Stage::GridComputeEnd, 1000 },
		                                                                                                                 { Stage::GridComputeHalted, 1000 },
		                                                                                                                 { Stage::GridComputeBegin, 1000 },
		                                                                                                                 { Stage::Rendering, 4 },
	                                                                                                                 },
	                                                                                                                 simulationShaders);

	simulation.System->DebugMaterial = marchingSqaureDebugMaterial;

	// Rendering
	ecs.AddSystem<System::RenderingPreparation>(Stage::Rendering, 0);
	ecs.AddSystem<System::PixelGridRenderer>(Stage::Rendering, 1, pixelGridMaterial);
	ecs.AddSystem<System::SpriteRenderer>(Stage::Rendering, 2, spriteMaterial, packingData);
	//ecs.AddSystem<System::PhysicsDebugRenderer>({ { Stage::Physics, 1000 }, { Stage::Rendering, 3 } }, physicsDebugMaterial, marchingSqaureDebugMaterial, physicsHandle);

	// Pre Rendering End
	ecs.AddSystem<System::ImGuiManager>(EngineStage::EndRendering, EngineStageOrder::EndRendering_RenderingSystem - 1);

	// Create entities
	uint64_t camera = ecs.CreateEntity<Component::Transform, Component::Camera, Component::SandboxController>({ { 0.0f, 0.0f, 10.0f } }, { 10.0f }, {});
	ecs.GetContextProvider().GetContext<Context::Global>()->CameraEntityID = camera;

	ecs.SetPrimaryGroup(Level::MainMenu);

	// Run Game
	application.Run();

	return 0;
}
