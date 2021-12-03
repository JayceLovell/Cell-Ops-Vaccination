#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>
#include <sstream>
#include <typeindex>
#include <optional>
#include <string>

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

// Graphics
#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture2D.h"
#include "Graphics/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

/// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/PlayerBehaviour.h"
#include "Gameplay/Components/EnemyBehaviour.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"


//#define LOG_GL_NOTIFICATIONS

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
	case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
	case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
	case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
	case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
#ifdef LOG_GL_NOTIFICATIONS
	case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
#endif
	default: break;
	}
}

// Stores our GLFW window in a global variable for now
GLFWwindow* window;
// The current size of our window in pixels
glm::ivec2 windowSize = glm::ivec2(800, 800);
// The title of our GLFW window
std::string windowTitle = "Cell Ops Vaccination";

// using namespace should generally be avoided, and if used, make sure it's ONLY in cpp files
using namespace Gameplay;
using namespace Gameplay::Physics;

// The scene that we will be rendering
Scene::Sptr scene = nullptr;

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowSize = glm::ivec2(width, height);
	if (windowSize.x * windowSize.y > 0) {
		scene->MainCamera->ResizeWindow(width, height);
	}
}

/// <summary>
/// Handles intializing GLFW, should be called before initGLAD, but after Logger::Init()
/// Also handles creating the GLFW window
/// </summary>
/// <returns>True if GLFW was initialized, false if otherwise</returns>
bool initGLFW() {
	// Initialize GLFW
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

	//Create a new GLFW window and make it current
	window = glfwCreateWindow(windowSize.x, windowSize.y, windowTitle.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	// Pass the window to the input engine and let it initialize itself
	InputEngine::Init(window);

	GuiBatcher::SetWindowSize(windowSize);

	return true;
}

/// <summary>
/// Handles initializing GLAD and preparing our GLFW window for OpenGL calls
/// </summary>
/// <returns>True if GLAD is loaded, false if there was an error</returns>
bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

/// <summary>
/// Draws a widget for saving or loading our scene
/// </summary>
/// <param name="scene">Reference to scene pointer</param>
/// <param name="path">Reference to path string storage</param>
/// <returns>True if a new scene has been loaded</returns>
bool DrawSaveLoadImGui(Scene::Sptr& scene, std::string& path) {
	// Since we can change the internal capacity of an std::string,
	// we can do cool things like this!
	ImGui::InputText("Path", path.data(), path.capacity());

	// Draw a save button, and save when pressed
	if (ImGui::Button("Save")) {
		scene->Save(path);

		std::string newFilename = std::filesystem::path(path).stem().string() + "-manifest.json";
		ResourceManager::SaveManifest(newFilename);
	}
	ImGui::SameLine();
	// Load scene from file button
	if (ImGui::Button("Load")) {
		// Since it's a reference to a ptr, this will
		// overwrite the existing scene!
		scene = nullptr;

		std::string newFilename = std::filesystem::path(path).stem().string() + "-manifest.json";
		ResourceManager::LoadManifest(newFilename);
		scene = Scene::Load(path);

		return true;
	}
	return false;
}

/// <summary>
/// Draws some ImGui controls for the given light
/// </summary>
/// <param name="title">The title for the light's header</param>
/// <param name="light">The light to modify</param>
/// <returns>True if the parameters have changed, false if otherwise</returns>
bool DrawLightImGui(const Scene::Sptr& scene, const char* title, int ix) {
	bool isEdited = false;
	bool result = false;
	Light& light = scene->Lights[ix];
	ImGui::PushID(&light); // We can also use pointers as numbers for unique IDs
	if (ImGui::CollapsingHeader(title)) {
		isEdited |= ImGui::DragFloat3("Pos", &light.Position.x, 0.01f);
		isEdited |= ImGui::ColorEdit3("Col", &light.Color.r);
		isEdited |= ImGui::DragFloat("Range", &light.Range, 0.1f);

		result = ImGui::Button("Delete");
	}
	if (isEdited) {
		scene->SetShaderLight(ix);
	}

	ImGui::PopID();
	return result;
}

/// <summary>
/// Draws a simple window for displaying materials and their editors
/// </summary>
void DrawMaterialsWindow() {
	if (ImGui::Begin("Materials")) {
		ResourceManager::Each<Material>([](Material::Sptr material) {
			material->RenderImGui();
			});
	}
	ImGui::End();
}

// <summary>
/// handles creating or loading the scene
/// </summary>
void CreateScene() {
	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene) {
		ResourceManager::LoadManifest("manifest.json");
		scene = Scene::Load("scene.json");

		// Call scene awake to start up all of our components
		scene->Window = window;
		scene->Awake();
	}
	else {
		////////////////////////////// SHADERS  ////////////////////////////////
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		Shader::Sptr reflectiveShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});

		// This shader handles our basic materials without reflections (cause they expensive)
		Shader::Sptr basicShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});

		// This shader handles our basic materials without reflections (cause they expensive)
		Shader::Sptr specShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});

		// This shader handles our foliage vertex shader example
		Shader::Sptr foliageShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});

		// This shader handles our cel shading example
		Shader::Sptr toonShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});


		/////////////////////////////////////////// MESHES ////////////////////////////////////////////////
		// Load in the meshes
		MeshResource::Sptr PlayerMesh = ResourceManager::CreateAsset<MeshResource>("models/Player.obj");
		MeshResource::Sptr LargeEnemyMesh = ResourceManager::CreateAsset<MeshResource>("models/Lower Poly Large Enemy.obj");
		MeshResource::Sptr FastEnemyMesh = ResourceManager::CreateAsset<MeshResource>("models/Lower Poly Fast Enemy.obj");
		MeshResource::Sptr NormalEnemyMesh = ResourceManager::CreateAsset<MeshResource>("models/Lower Poly Normal Enemy.obj");
		MeshResource::Sptr LevelMesh = ResourceManager::CreateAsset<MeshResource>("models/Game Floor.obj");
		MeshResource::Sptr LungsTargetMesh = ResourceManager::CreateAsset<MeshResource>("models/LungsTarget.obj");
		MeshResource::Sptr CellMesh = ResourceManager::CreateAsset<MeshResource>("models/Cell.obj");
		MeshResource::Sptr Cell2Mesh = ResourceManager::CreateAsset<MeshResource>("models/Cell2.obj");
		MeshResource::Sptr Co2Mesh = ResourceManager::CreateAsset<MeshResource>("models/Co2.obj");
		MeshResource::Sptr OxygenMesh = ResourceManager::CreateAsset<MeshResource>("models/Oxygen.obj");

		MeshResource::Sptr APCMesh = ResourceManager::CreateAsset<MeshResource>("models/APC.obj");
		MeshResource::Sptr APC2Mesh = ResourceManager::CreateAsset<MeshResource>("models/APC2.obj");
		MeshResource::Sptr SymbiontMesh = ResourceManager::CreateAsset<MeshResource>("models/Symbiont.obj");
		MeshResource::Sptr Symbiont2Mesh = ResourceManager::CreateAsset<MeshResource>("models/Symbiont2.obj");
		MeshResource::Sptr VeinMesh = ResourceManager::CreateAsset<MeshResource>("models/Vein.obj");

		/////////////////////////////////////////// TEXTURES ////////////////////////////////////////////////
		// Load in some textures
		Texture2D::Sptr		PlayerTexture = ResourceManager::CreateAsset<Texture2D>("textures/tempWhiteCell.jpg");
		Texture2D::Sptr		LargeEnemyTexture = ResourceManager::CreateAsset<Texture2D>("textures/Large Enemy.png");
		Texture2D::Sptr		FastEnemyTexture = ResourceManager::CreateAsset<Texture2D>("textures/Fast Enemy.png");
		Texture2D::Sptr		NormalEnemyTexture = ResourceManager::CreateAsset<Texture2D>("textures/Normal Enemy.png");
		Texture2D::Sptr		LevelTexture = ResourceManager::CreateAsset<Texture2D>("textures/Lungs_Floor_Asset_Small.png");
		Texture2D::Sptr		LungTexture = ResourceManager::CreateAsset<Texture2D>("textures/LungTexture.jpg");
		Texture2D::Sptr		CellTexture = ResourceManager::CreateAsset<Texture2D>("textures/Cell.png");
		Texture2D::Sptr		Cell2Texture = ResourceManager::CreateAsset<Texture2D>("textures/Cell2.png");
		Texture2D::Sptr		Co2Texture = ResourceManager::CreateAsset<Texture2D>("textures/Co2.png");
		Texture2D::Sptr		OxygenTexture = ResourceManager::CreateAsset<Texture2D>("textures/Oxygen.png");

		Texture2D::Sptr		APCTexture = ResourceManager::CreateAsset<Texture2D>("textures/APC.png");
		Texture2D::Sptr		APC2Texture = ResourceManager::CreateAsset<Texture2D>("textures/APC2.png");
		Texture2D::Sptr		SymbiontTexture = ResourceManager::CreateAsset<Texture2D>("textures/Symbiont.png");
		Texture2D::Sptr		Symbiont2Texture = ResourceManager::CreateAsset<Texture2D>("textures/Symbiont2.png");
		Texture2D::Sptr		VeinTexture = ResourceManager::CreateAsset<Texture2D>("textures.Vein.png");

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		Shader::Sptr      skyboxShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Create an empty scene
		scene = std::make_shared<Scene>();

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap);
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Create our materials
		// This will be our box material, with no environment reflections

		Material::Sptr PlayerMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			PlayerMaterial->Name = "PlayerMaterial";
			PlayerMaterial->Set("u_Material.Diffuse", PlayerTexture);
			PlayerMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr LargeEnemyMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			LargeEnemyMaterial->Name = "LargeEnemyMaterial";
			LargeEnemyMaterial->Set("u_Material.Diffuse", LargeEnemyTexture);
			LargeEnemyMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr NormalEnemyMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			NormalEnemyMaterial->Name = "NormalEnemyMaterial";
			NormalEnemyMaterial->Set("u_Material.Diffuse", NormalEnemyTexture);
			NormalEnemyMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr FastEnemyMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			FastEnemyMaterial->Name = "FastEnemyMaterial";
			FastEnemyMaterial->Set("u_Material.Diffuse", FastEnemyTexture);
			FastEnemyMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr LevelMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			LevelMaterial->Name = "Levelmaterial";
			LevelMaterial->Set("u_Material.Diffuse", LevelTexture);
			LevelMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr LungMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			LungMaterial->Name = "LungMaterial";
			LungMaterial->Set("u_Material.Diffuse",LungTexture);
			LungMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr CellMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			CellMaterial->Name = "CellMateriall";
			CellMaterial->Set("u_Material.Diffuse", CellTexture);
			CellMaterial->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr Cell2Material = ResourceManager::CreateAsset<Material>(basicShader);
		{
			CellMaterial->Name = "Cell2Materiall";
			CellMaterial->Set("u_Material.Diffuse", Cell2Texture);
			CellMaterial->Set("u_Material.Shininess", 0.1f);
		}

		Material::Sptr Co2Material = ResourceManager::CreateAsset<Material>(basicShader);
		{
			Co2Material->Name = "Co2Material";
			Co2Material->Set("u_Material.Diffuse", Co2Texture);
			Co2Material->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr OxygenMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			OxygenMaterial->Name = "OxygenMaterial";
			OxygenMaterial->Set("u_Material.Diffuse", OxygenTexture);
			OxygenMaterial->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr APCMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			APCMaterial->Name = "APCMaterial";
			APCMaterial->Set("u_Material.Diffuse", APCTexture);
			APCMaterial->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr APC2Material = ResourceManager::CreateAsset<Material>(basicShader);
		{
			APC2Material->Name = "APC2Material";
			APC2Material->Set("u_Material.Diffuse", APC2Texture);
			APC2Material->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr SymbiontMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			SymbiontMaterial->Name = "SymbiontMaterial";
			SymbiontMaterial->Set("u_Material.Diffuse", SymbiontTexture);
			SymbiontMaterial->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr Symbiont2Material = ResourceManager::CreateAsset<Material>(basicShader);
		{
			Symbiont2Material->Name = "Symbiont2Material";
			Symbiont2Material->Set("u_Material.Diffuse", Symbiont2Texture);
			Symbiont2Material->Set("u_Material.Shininess", 0.1f);
		}
		Material::Sptr VeinMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			VeinMaterial->Name = "VeinMaterial";
			VeinMaterial->Set("u_Material.Diffuse", VeinTexture);
			VeinMaterial->Set("u_Material.Shininess", 0.1f);
		}

		/////////////// MAP MATERIALS ////////////////////

		/*Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.Diffuse", diffuseMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("s_NormalMap", normalMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(tangentSpaceMapping);
		{
			Texture2D::Sptr normalMap = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.Diffuse", diffuseMap);
			normalmapMat->Set("s_NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f);
		}*/


		// Create some lights for our scene
		scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(50.0f, 10.0f, 10.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 100.0f;

		/*scene->Lights[1].Position = glm::vec3(1.0f, 0.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.2f, 0.8f, 0.1f);

		scene->Lights[2].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(1.0f, 0.2f, 0.1f);*/

		 //Set up the scene's camera For Debugging
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
		{
			camera->SetPostion(glm::vec3(5.0f));
			camera->SetRotation(glm::vec3(112.735f, 0.0f, -72.0f));
			//camera->LookAt(glm::vec3(0.0f));

			camera->Add<SimpleCameraControl>();

			Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			scene->MainCamera = cam;
		}
		GameObject::Sptr Player = scene->CreateGameObject("Player");
		{
			// Add a render component
			RenderComponent::Sptr renderer = Player->Add<RenderComponent>();

			renderer->SetMesh(PlayerMesh);
			renderer->SetMaterial(PlayerMaterial);
			
			Player->SetPostion(glm::vec3(100.0f));
			
			Player->Add<PlayerBehaviour>();
			

			TriggerVolume::Sptr trigger = Player->Add<TriggerVolume>();
			ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();
			//trigger->SetFlags(TriggerTypeFlags::Statics | TriggerTypeFlags::Kinematics);
			trigger->AddCollider(collider);
		}

		GameObject::Sptr Level = scene->CreateGameObject("Level");
		{

			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			RenderComponent::Sptr renderer =Level->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(LevelMaterial);

			// Attach a plane collider that extends infinitely along the X / Y axis
			RigidBody::Sptr physics = Level->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}
		GameObject::Sptr Target = scene->CreateGameObject("Target");
		{
			Target->SetPostion(glm::vec3(20.f, 0.0f, 0.0f));

			// Add a render component
			RenderComponent::Sptr renderer = Target->Add<RenderComponent>();
			renderer->SetMesh(LungsTargetMesh);
			renderer->SetMaterial(LungMaterial);

			// Add a dynamic rigid body to this monkey
			RigidBody::Sptr physics = Target->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(ConvexMeshCollider::Create());

			TriggerVolume::Sptr volume = Target->Add<TriggerVolume>();
			ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();
			volume->AddCollider(collider);

		}
		////////////////////////Enemies///////////////////////////////
		//GameObject::Sptr LargeEnemy = scene->CreateGameObject("LargeEnemy");
		//{
		//	// Set and rotation position in the scene
		//	LargeEnemy->SetPostion(glm::vec3(30.0f, 10.0f, 10.0f));			

		//	// Add a render component
		//	RenderComponent::Sptr renderer = LargeEnemy->Add<RenderComponent>();
		//	renderer->SetMesh(LargeEnemyMesh);
		//	renderer->SetMaterial(LargeEnemyMaterial);

		//	// Add a dynamic rigid body to this monkey
		//	RigidBody::Sptr physics = LargeEnemy->Add<RigidBody>(RigidBodyType::Dynamic);
		//	physics->SetMass(0.0f);
		//	physics->AddCollider(ConvexMeshCollider::Create());


		//	TriggerVolume::Sptr trigger = LargeEnemy->Add<TriggerVolume>();
		//	trigger->AddCollider(ConvexMeshCollider::Create());

		//	LargeEnemy->Add<EnemyBehaviour>();
		//	LargeEnemy->Get<EnemyBehaviour>()->EnemyType = "Large Enemy";
		//	LargeEnemy->Get<EnemyBehaviour>()->_maxHealth = 50;
		//}
		
		//GameObject::Sptr FastEnemy = scene->CreateGameObject("FastEnemy");
		//{
		//	// Set and rotation position in the scene
		//	FastEnemy->SetPostion(glm::vec3(10.0f, 10.0f, 10.0f));

		//	// Add a render component
		//	RenderComponent::Sptr renderer = FastEnemy->Add<RenderComponent>();
		//	renderer->SetMesh(FastEnemyMesh);
		//	renderer->SetMaterial(FastEnemyMaterial);

		//	// Add a dynamic rigid body to this monkey
		//	RigidBody::Sptr physics = FastEnemy->Add<RigidBody>(RigidBodyType::Dynamic);
		//	physics->SetMass(0.0f);
		//	physics->AddCollider(ConvexMeshCollider::Create());


		//	TriggerVolume::Sptr trigger = FastEnemy->Add<TriggerVolume>();
		//	trigger->AddCollider(ConvexMeshCollider::Create());

		//	FastEnemy->Add<EnemyBehaviour>();
		//	FastEnemy->Get<EnemyBehaviour>()->EnemyType = "Fast Enemy";
		//	FastEnemy->Get<EnemyBehaviour>()->_maxHealth = 10;
		//}

		GameObject::Sptr NormalEnemy = scene->CreateGameObject("NormalEnemy");
		{
			// Set and rotation position in the scene
			NormalEnemy->SetPostion(glm::vec3(20.0f, 10.0f, 10.0f));

			// Add a render component
			RenderComponent::Sptr renderer = NormalEnemy->Add<RenderComponent>();
			renderer->SetMesh(NormalEnemyMesh);
			renderer->SetMaterial(NormalEnemyMaterial);

			// Add a dynamic rigid body to this monkey
			RigidBody::Sptr physics = NormalEnemy->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->SetMass(0.0f);
			physics->AddCollider(ConvexMeshCollider::Create());


			TriggerVolume::Sptr trigger = NormalEnemy->Add<TriggerVolume>();
			trigger->AddCollider(ConvexMeshCollider::Create());

			NormalEnemy->Add<EnemyBehaviour>();
			NormalEnemy->Get<EnemyBehaviour>()->EnemyType = "Normal Enemy";
			NormalEnemy->Get<EnemyBehaviour>()->_maxHealth = 30;
		}

		//////////////// Background Objects

		GameObject::Sptr Cell = scene->CreateGameObject("Cell");
		{
			Cell->SetPostion(glm::vec3(40.0f, 10.0f, 10.0f));


			// Add a render component
			RenderComponent::Sptr renderer = Cell->Add<RenderComponent>();
			renderer->SetMesh(CellMesh);
			renderer->SetMaterial(CellMaterial);

		}
		GameObject::Sptr Cell2 = scene->CreateGameObject("Cell2");
		{
			Cell2->SetPostion(glm::vec3(40.0f, 20.0f, 10.0f));


			// Add a render component
			RenderComponent::Sptr renderer = Cell2->Add<RenderComponent>();
			renderer->SetMesh(Cell2Mesh);
			renderer->SetMaterial(Cell2Material);

		}
		GameObject::Sptr Co2 = scene->CreateGameObject("Co2");
		{
			Co2->SetPostion(glm::vec3(50.0f, 10.0f, 10.0f));


			// Add a render component
			RenderComponent::Sptr renderer = Co2->Add<RenderComponent>();
			renderer->SetMesh(Co2Mesh);
			renderer->SetMaterial(Co2Material);
		}
		GameObject::Sptr Oxygen = scene->CreateGameObject("Oxygen");
		{
			Oxygen->SetPostion(glm::vec3(60.0f, 10.0f, 10.0f));


			// Add a render component
			RenderComponent::Sptr renderer = Oxygen->Add<RenderComponent>();
			renderer->SetMesh(OxygenMesh);
			renderer->SetMaterial(OxygenMaterial);

		}
		GameObject::Sptr APC = scene->CreateGameObject("APC");
		{
			APC->SetPostion(glm::vec3(70.0f, 10.0f, 10.0f));


			// Add a render component
			RenderComponent::Sptr renderer = APC->Add<RenderComponent>();
			renderer->SetMesh(APCMesh);
			renderer->SetMaterial(APCMaterial);

		}
		GameObject::Sptr APC2 = scene->CreateGameObject("APC2");
		{
			APC2->SetPostion(glm::vec3(70.0f, 20.0f, 10.0f));


			// Add a render component
			RenderComponent::Sptr renderer = APC2->Add<RenderComponent>();
			renderer->SetMesh(APC2Mesh);
			renderer->SetMaterial(APC2Material);

		}
		GameObject::Sptr Symbiont = scene->CreateGameObject("Symbiont");
		{
			Symbiont->SetPostion(glm::vec3(80.0f, 10.0f, 10.0f));


			// Add a render component
			RenderComponent::Sptr renderer = Symbiont->Add<RenderComponent>();
			renderer->SetMesh(SymbiontMesh);
			renderer->SetMaterial(SymbiontMaterial);

		}
		GameObject::Sptr Symbiont2 = scene->CreateGameObject("Symbiont2");
		{
			Symbiont2->SetPostion(glm::vec3(80.0f, 20.0f, 10.0f));


			// Add a render component
			RenderComponent::Sptr renderer = Symbiont2->Add<RenderComponent>();
			renderer->SetMesh(Symbiont2Mesh);
			renderer->SetMaterial(Symbiont2Material);

		}
		/////////////////////////// UI //////////////////////////////
		//If when you uncomment this code remmeber to uncomment RenderGUI at line 955(proably pushed up or down by time of reading this but its around there 
		/*GameObject::Sptr canvas = scene->CreateGameObject("UI Canvas");
		{
			RectTransform::Sptr transform = canvas->Add<RectTransform>();
			transform->SetMin({ 16, 16 });
			transform->SetMax({ 256, 256 });

			GuiPanel::Sptr canPanel = canvas->Add<GuiPanel>();

			GameObject::Sptr subPanel = scene->CreateGameObject("Sub Item");
			{
				RectTransform::Sptr transform = subPanel->Add<RectTransform>();
				transform->SetMin({ 10, 10 });
				transform->SetMax({ 128, 128 });

				GuiPanel::Sptr panel = subPanel->Add<GuiPanel>();
				panel->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

				Font::Sptr font = ResourceManager::CreateAsset<Font>("fonts/Roboto-Medium.ttf", 16.0f);
				font->Bake();

				GuiText::Sptr text = subPanel->Add<GuiText>();
				text->SetText("Hello world!");
				text->SetFont(font);
			}

			canvas->AddChild(subPanel);
		}*/

		/*GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);*/

		// Call scene awake to start up all of our components
		scene->Window = window;
		scene->Awake();

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");
	}
}

//// Templated LERP function
//template<typename T>
//T LERP(const T& p0, const T& p1, float t)
//{
//	return (1.0f - t) * p0 + t * p1;
//}

int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Initialize our ImGui helper
	ImGuiHelper::Init(window);

	// Initialize our resource manager
	ResourceManager::Init();

	// Register all our resource types so we can load them from manifest files
	ResourceManager::RegisterType<Texture2D>();
	ResourceManager::RegisterType<TextureCube>();
	ResourceManager::RegisterType<Shader>();
	ResourceManager::RegisterType<Material>();
	ResourceManager::RegisterType<MeshResource>();

	// Register all of our component types so we can load them from files
	ComponentManager::RegisterType<Camera>();
	ComponentManager::RegisterType<RenderComponent>();
	ComponentManager::RegisterType<RigidBody>();
	ComponentManager::RegisterType<TriggerVolume>();
	ComponentManager::RegisterType<JumpBehaviour>();
	ComponentManager::RegisterType<MaterialSwapBehaviour>();
	ComponentManager::RegisterType<TriggerVolumeEnterBehaviour>();
	ComponentManager::RegisterType<SimpleCameraControl>();
	ComponentManager::RegisterType<PlayerBehaviour>();
	ComponentManager::RegisterType<EnemyBehaviour>();

	ComponentManager::RegisterType<RectTransform>();
	ComponentManager::RegisterType<GuiPanel>();
	ComponentManager::RegisterType<GuiText>();

	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);


	// Structure for our frame-level uniforms, matches layout from
	// fragments/frame_uniforms.glsl
	// For use with a UBO.
	struct FrameLevelUniforms {
		// The camera's view matrix
		glm::mat4 u_View;
		// The camera's projection matrix
		glm::mat4 u_Projection;
		// The combined viewProject matrix
		glm::mat4 u_ViewProjection;
		// The camera's position in world space
		glm::vec4 u_CameraPos;
		// The time in seconds since the start of the application
		float u_Time;
	};
	// This uniform buffer will hold all our frame level uniforms, to be shared between shaders
	UniformBuffer<FrameLevelUniforms>::Sptr frameUniforms = std::make_shared<UniformBuffer<FrameLevelUniforms>>(BufferUsage::DynamicDraw);
	// The slot that we'll bind our frame level UBO to
	const int FRAME_UBO_BINDING = 0;

	// Structure for our isntance-level uniforms, matches layout from
	// fragments/frame_uniforms.glsl
	// For use with a UBO.
	struct InstanceLevelUniforms {
		// Complete MVP
		glm::mat4 u_ModelViewProjection;
		// Just the model transform, we'll do worldspace lighting
		glm::mat4 u_Model;
		// Normal Matrix for transforming normals
		glm::mat4 u_NormalMatrix;
	};

	// This uniform buffer will hold all our instance level uniforms, to be shared between shaders
	UniformBuffer<InstanceLevelUniforms>::Sptr instanceUniforms = std::make_shared<UniformBuffer<InstanceLevelUniforms>>(BufferUsage::DynamicDraw);

	// The slot that we'll bind our instance level UBO to
	const int INSTANCE_UBO_BINDING = 1;

	////////////////////////////////
	///// SCENE CREATION MOVED /////
	////////////////////////////////
	CreateScene();

	// We'll use this to allow editing the save/load path
	// via ImGui, note the reserve to allocate extra space
	// for input!
	std::string scenePath = "scene.json";
	scenePath.reserve(256);

	// Our high-precision timer
	double lastFrame = glfwGetTime();

	BulletDebugMode physicsDebugMode = BulletDebugMode::None;
	float playbackSpeed = 1.0f;

	nlohmann::json editorSceneState;

	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGuiHelper::StartFrame();

		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);



		// Draw our material properties window!
		DrawMaterialsWindow();

		// Showcasing how to use the imGui library!
		bool isDebugWindowOpen = ImGui::Begin("Debugging");
		if (isDebugWindowOpen) {
			// Draws a button to control whether or not the game is currently playing
			static char buttonLabel[64];
			sprintf_s(buttonLabel, "%s###playmode", scene->IsPlaying ? "Exit Play Mode" : "Enter Play Mode");
			if (ImGui::Button(buttonLabel)) {
				// Save scene so it can be restored when exiting play mode
				if (!scene->IsPlaying) {
					editorSceneState = scene->ToJson();
				}

				// Toggle state
				scene->IsPlaying = !scene->IsPlaying;

				// If we've gone from playing to not playing, restore the state from before we started playing
				if (!scene->IsPlaying) {
					scene = nullptr;
					// We reload to scene from our cached state
					scene = Scene::FromJson(editorSceneState);
					// Don't forget to reset the scene's window and wake all the objects!
					scene->Window = window;
					scene->Awake();
				}
			}

			// Make a new area for the scene saving/loading
			ImGui::Separator();
			if (DrawSaveLoadImGui(scene, scenePath)) {
				// C++ strings keep internal lengths which can get annoying
				// when we edit it's underlying datastore, so recalcualte size
				scenePath.resize(strlen(scenePath.c_str()));

				// We have loaded a new scene, call awake to set
				// up all our components
				scene->Window = window;
				scene->Awake();
			}
			ImGui::Separator();
			// Draw a dropdown to select our physics debug draw mode
			if (BulletDebugDraw::DrawModeGui("Physics Debug Mode:", physicsDebugMode)) {
				scene->SetPhysicsDebugDrawMode(physicsDebugMode);
			}
			LABEL_LEFT(ImGui::SliderFloat, "Playback Speed:    ", &playbackSpeed, 0.0f, 10.0f);
			ImGui::Separator();
		}

		// Clear the color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw some ImGui stuff for the lights
		if (isDebugWindowOpen) {
			for (int ix = 0; ix < scene->Lights.size(); ix++) {
				char buff[256];
				sprintf_s(buff, "Light %d##%d", ix, ix);
				// DrawLightImGui will return true if the light was deleted
				if (DrawLightImGui(scene, buff, ix)) {
					// Remove light from scene, restore all lighting data
					scene->Lights.erase(scene->Lights.begin() + ix);
					scene->SetupShaderAndLights();
					// Move back one element so we don't skip anything!
					ix--;
				}
			}
			// As long as we don't have max lights, draw a button
			// to add another one
			if (scene->Lights.size() < scene->MAX_LIGHTS) {
				if (ImGui::Button("Add Light")) {
					scene->Lights.push_back(Light());
					scene->SetupShaderAndLights();
				}
			}
			// Split lights from the objects in ImGui
			ImGui::Separator();
		}

		dt *= playbackSpeed;

		// Perform updates for all components
		scene->Update(dt);

		// Grab shorthands to the camera and shader from the scene
		Camera::Sptr camera = scene->MainCamera;

		// Cache the camera's viewprojection
		glm::mat4 viewProj = camera->GetViewProjection();
		DebugDrawer::Get().SetViewProjection(viewProj);

		// Make sure depth testing and culling are re-enabled
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		// Update our worlds physics!
		scene->DoPhysics(dt);

		// Draw object GUIs
		if (isDebugWindowOpen) {
			scene->DrawAllGameObjectGUIs();
		}

		// The current material that is bound for rendering
		Material::Sptr currentMat = nullptr;
		Shader::Sptr shader = nullptr;

		// Bind the skybox texture to a reserved texture slot
		// See Material.h and Material.cpp for how we're reserving texture slots
		TextureCube::Sptr environment = scene->GetSkyboxTexture();
		if (environment) environment->Bind(0);


		// Here we'll bind all the UBOs to their corresponding slots
		scene->PreRender();
		frameUniforms->Bind(FRAME_UBO_BINDING);
		instanceUniforms->Bind(INSTANCE_UBO_BINDING);

		// Upload frame level uniforms
		auto& frameData = frameUniforms->GetData();
		frameData.u_Projection = camera->GetProjection();
		frameData.u_View = camera->GetView();
		frameData.u_ViewProjection = camera->GetViewProjection();
		frameData.u_CameraPos = glm::vec4(camera->GetGameObject()->GetPosition(), 1.0f);
		frameData.u_Time = static_cast<float>(thisFrame);
		frameUniforms->Update();

		// Render all our objects
		ComponentManager::Each<RenderComponent>([&](const RenderComponent::Sptr& renderable) {
			// Early bail if mesh not set
			if (renderable->GetMesh() == nullptr) {
				return;
			}

			// If we don't have a material, try getting the scene's fallback material
			// If none exists, do not draw anything
			if (renderable->GetMaterial() == nullptr) {
				if (scene->DefaultMaterial != nullptr) {
					renderable->SetMaterial(scene->DefaultMaterial);
				}
				else {
					return;
				}
			}

			// If the material has changed, we need to bind the new shader and set up our material and frame data
			// Note: This is a good reason why we should be sorting the render components in ComponentManager
			if (renderable->GetMaterial() != currentMat) {
				currentMat = renderable->GetMaterial();
				shader = currentMat->GetShader();

				shader->Bind();
				currentMat->Apply();
			}

			// Grab the game object so we can do some stuff with it
			GameObject* object = renderable->GetGameObject();

			// Use our uniform buffer for our instance level uniforms
			auto& instanceData = instanceUniforms->GetData();
			instanceData.u_Model = object->GetTransform();
			instanceData.u_ModelViewProjection = viewProj * object->GetTransform();
			instanceData.u_NormalMatrix = glm::mat3(glm::transpose(glm::inverse(object->GetTransform())));
			instanceUniforms->Update();

			// Draw the object
			renderable->GetMesh()->Draw();
			});

		// Use our cubemap to draw our skybox
		scene->DrawSkybox();

		// Disable culling
		glDisable(GL_CULL_FACE);
		// Disable depth testing, we're going to use order-dependant layering
		glDisable(GL_DEPTH_TEST);
		// Disable depth writing
		glDepthMask(GL_FALSE);

		// Enable alpha blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Enable the scissor test;
		glEnable(GL_SCISSOR_TEST);

		// Our projection matrix will be our entire window for now
		glm::mat4 proj = glm::ortho(0.0f, (float)windowSize.x, (float)windowSize.y, 0.0f, -1.0f, 1.0f);
		GuiBatcher::SetProjection(proj);

		// Iterate over and render all the GUI objects
		//scene->RenderGUI();

		// Flush the Gui Batch renderer
		GuiBatcher::Flush();

		// Disable alpha blending
		glDisable(GL_BLEND);
		// Disable scissor testing
		glDisable(GL_SCISSOR_TEST);
		// Re-enable depth writing
		glDepthMask(GL_TRUE);

		// End our ImGui window
		ImGui::End();

		VertexArrayObject::Unbind();

		lastFrame = thisFrame;
		ImGuiHelper::EndFrame();
		InputEngine::EndFrame();
		glfwSwapBuffers(window);
	}

	// Clean up the ImGui library
	ImGuiHelper::Cleanup();

	// Clean up the resource manager
	ResourceManager::Cleanup();

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}