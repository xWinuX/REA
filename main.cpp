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
#include "REA/System/GameOfLifeSimulation.hpp"


using namespace SplitEngine;
using namespace REA;

int main()
{
	Application application = Application({});

	application.GetWindow().SetSize(1000, 1000);

	Input::RegisterAxis2D(InputAction::Move, { KeyCode::A, KeyCode::D }, { KeyCode::S, KeyCode::W });
	Input::RegisterButtonAction(InputAction::Fire, KeyCode::MOUSE_LEFT);

	AssetDatabase& assetDatabase = application.GetAssetDatabase();

	/**
	 * You can create an Asset with every type that has a public struct named CreateInfo!
	 * That means you can also create custom assets
	 * The key can be anything as long as it can be cast into an int, so no magic strings!
	 */
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


	AssetHandle<Rendering::Texture2D> carstenTexture = assetDatabase.CreateAsset<Rendering::Texture2D>(Texture::Carsten, {IO::ImageLoader::Load("res/textures/Carsten.png")});

	// Setup ECS
	ECS::Registry& ecs = application.GetECSRegistry();

	/**
	 * 	Each component that is used needs to be registered
	 * 	It's important that every Component, that will be used, is registered before any Entities are created or else the ECS will not work or likely crash the app
	 */
	ecs.RegisterComponent<Component::Transform>();
	ecs.RegisterComponent<Component::SpriteRenderer>();
	ecs.RegisterComponent<Component::Physics>();
	ecs.RegisterComponent<Component::AudioSource>();
	ecs.RegisterComponent<Component::Player>();
	ecs.RegisterComponent<Component::Camera>();
	ecs.RegisterComponent<Component::PixelGrid>();

	/**
	 * 	Each system can either be registered as a gameplay system or as a render system
	 * 	Gameplay systems execute before rendering systems and don't have any special context
	 * 	Render systems execute after gameplay systems and run in a vulkan context so draw calls/binds etc... can be made
	 * 	Systems can be registered multiple times and parameters get forwarded to the systems constructor
	 * 	Systems can be registered as the game is running, so no need to preregister all at the start if you don't want to
	 */
	ecs.AddSystem<System::Debug>(ECS::Stage::Gameplay, -1);
	ecs.AddSystem<System::PixelGridDrawing>(ECS::Stage::Gameplay,0);
	ecs.AddSystem<System::Camera>(ECS::Stage::Gameplay, 1);
	ecs.AddSystem<System::AudioSourcePlayer>(ECS::Stage::Gameplay, 999);

	ecs.AddSystem<System::PixelGridSimulation>(ECS::Stage::Gameplay, 999);
	//ecs.AddSystem<System::GameOfLifeSimulation>(ECS::Stage::Gameplay, 999);

	ecs.AddSystem<System::RenderingPreparation>(ECS::Stage::Rendering, 0);
	ecs.AddSystem<System::PixelGridRenderer>(ECS::Stage::Rendering, 1, pixelGridMaterial, carstenTexture);
	//ecs.AddSystem<System::SpriteRenderer>(ECS::Stage::Rendering, 2, spriteMaterial, packingData);

	// Create entities
	uint64_t playerEntity = ecs.CreateEntity<Component::Transform, Component::Physics, Component::Player, Component::SpriteRenderer>({}, {}, {}, { floppaSprite, 1.0f });


	ecs.CreateEntity<Component::Transform, Component::Camera>({}, { playerEntity });


	Component::PixelGrid pixelGrid = {};
	uint64_t pixelGridEntity = ecs.CreateEntity<Component::PixelGrid>(std::move(pixelGrid));

	// Run Game
	application.Run();

	return 0;
}
