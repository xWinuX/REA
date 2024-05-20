#include <imgui.h>

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
#include "REA/Stage.hpp"
#include "REA/System/GameOfLifeSimulation.hpp"
#include "REA/System/ImGuiManager.hpp"


using namespace SplitEngine;
using namespace REA;

int main()
{
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
	AssetHandle<Rendering::Shader> pixelGridComputeFall = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Comp_PixelGrid_Fall, { "res/shaders/PixelGridCompute" });
	AssetHandle<Rendering::Shader> pixelGridComputeFlow = assetDatabase.CreateAsset<Rendering::Shader>(Shader::Comp_PixelGrid_Flow, { "res/shaders/PixelGridComputeFlow" });

	ECS::Registry& ecs = application.GetECSRegistry();

	ecs.RegisterComponent<Component::Transform>();
	ecs.RegisterComponent<Component::SpriteRenderer>();
	ecs.RegisterComponent<Component::Physics>();
	ecs.RegisterComponent<Component::AudioSource>();
	ecs.RegisterComponent<Component::Player>();
	ecs.RegisterComponent<Component::Camera>();
	ecs.RegisterComponent<Component::PixelGrid>();

	ecs.AddSystem<System::Debug>(Stage::Gameplay, -1);
	ecs.AddSystem<System::ImGuiManager>(EngineStage::EndRendering, EngineStageOrder::EndRendering_RenderingSystem - 1);

	ecs.AddSystem<System::PixelGridDrawing>(Stage::Gameplay, 0, 50);
	ecs.AddSystem<System::Camera>(Stage::Gameplay, 1);
	ecs.AddSystem<System::AudioSourcePlayer>(Stage::Gameplay, 999);


	System::PixelGridSimulation::SimulationShaders simulationShaders = {
		.IdleSimulation = pixelGridComputeIdle,
		.FallingSimulation = pixelGridComputeFall,
		.FlowSimulation = pixelGridComputeFlow
	};
	ecs.AddSystem<System::PixelGridSimulation>(Stage::Gameplay, 999, simulationShaders);
	//ecs.AddSystem<System::GameOfLifeSimulation>(ECS::Stage::Gameplay, 999);

	ecs.AddSystem<System::RenderingPreparation>(Stage::Rendering, 0);
	ecs.AddSystem<System::PixelGridRenderer>(Stage::Rendering, 1, pixelGridMaterial);
	//ecs.AddSystem<System::SpriteRenderer>(ECS::Stage::Rendering, 2, spriteMaterial, packingData);

	// Create entities
	uint64_t playerEntity = ecs.CreateEntity<Component::Transform, Component::Physics, Component::Player, Component::SpriteRenderer>({}, {}, {}, { floppaSprite, 1.0f });


	ecs.CreateEntity<Component::Transform, Component::Camera>({}, { playerEntity });


	Component::PixelGrid pixelGrid       = {};
	uint64_t             pixelGridEntity = ecs.CreateEntity<Component::PixelGrid>(std::move(pixelGrid));

	// Run Game
	application.Run();

	return 0;
}
