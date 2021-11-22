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

// Components
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

		/////////////////////////////////////////// TEXTURES ////////////////////////////////////////////////
		// Load in some textures
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    PlayerTexture = ResourceManager::CreateAsset<Texture2D>("textures/tempWhiteCell.jpg");
		Texture2D::Sptr    LargeEnemyTexture = ResourceManager::CreateAsset<Texture2D>("textures/Lower Poly Large Enemy Textured.png");
		Texture2D::Sptr    FastEnemyTexture = ResourceManager::CreateAsset<Texture2D>("textures/Lower Poly Fast Enemy Textured.png");
		Texture2D::Sptr    NormalEnemyTexture = ResourceManager::CreateAsset<Texture2D>("textures/Lower Poly Normal Enemy Textured.png");
		Texture2D::Sptr		LevelTexture = ResourceManager::CreateAsset<Texture2D>("textures/Vein Floor Textured.png");
		Texture2D::Sptr		LungTexture = ResourceManager::CreateAsset<Texture2D>("textures/LungTexture.jpg");

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
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.Diffuse", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
		}

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


		// Create some lights for our scene
		/*scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 100.0f;

		scene->Lights[1].Position = glm::vec3(1.0f, 0.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.2f, 0.8f, 0.1f);

		scene->Lights[2].Position = glm::vec3(0.0f, 1.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(1.0f, 0.2f, 0.1f);*/

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		 //Set up the scene's camera For Debugging
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
		{
			camera->SetPostion(glm::vec3(5.0f));
			camera->LookAt(glm::vec3(0.0f));

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
			

			//RigidBody::Sptr physics = Player->Add<RigidBody>(/*static by default*/);
			//physics->AddCollider(ConvexMeshCollider::Create());
			TriggerVolume::Sptr volume = Player->Add<TriggerVolume>();
			ConvexMeshCollider::Sptr collider = ConvexMeshCollider::Create();
			volume->AddCollider(collider);
		}

		//GameObject::Sptr WhiteCell = scene->CreateGameObject("WhiteCell"); {
		//	// Add a render component
		//	RenderComponent::Sptr renderer = WhiteCell->Add<RenderComponent>();
		//	renderer->SetMesh(WhiteCellMesh);
		//	renderer->SetMaterial(WhiteCellMaterial);

		//	WhiteCell->SetPostion(glm::vec3(0.0, 0.0, 10.0f));

		//	WhiteCell->Add<PlayerBehaviour>();
		//	WhiteCell->Add<JumpBehaviour>();

		//	// Add a dynamic rigid body to this monkey
		//	RigidBody::Sptr physics = WhiteCell->Add<RigidBody>(RigidBodyType::Dynamic);
		//	physics->AddCollider(ConvexMeshCollider::Create());
		//}

		// Set up all our sample objects
		GameObject::Sptr Level = scene->CreateGameObject("Level");
		{

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = Level->Add<RenderComponent>();
			renderer->SetMesh(LevelMesh);
			renderer->SetMaterial(LevelMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = Level->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(ConvexMeshCollider::Create());
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

		}
		//GameObject::Sptr LargeEnemy = scene->CreateGameObject("LargeEnemy");
		//{
		//	// Set and rotation position in the scene
		//	LargeEnemy->SetPostion(glm::vec3(30.0f, 1.0f, 10.0f));			

		//	// Add a render component
		//	RenderComponent::Sptr renderer = LargeEnemy->Add<RenderComponent>();
		//	renderer->SetMesh(LargeEnemyMesh);
		//	renderer->SetMaterial(LargeEnemyMaterial);

		//	//LargeEnemy->Add<EnemyBehaviour>();

		//	// This is an example of attaching a component and setting some parameters
		//	//RotatingBehaviour::Sptr behaviour = monkey2->Add<RotatingBehaviour>();
		//	//behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, -90.0f);

		//	// Add a dynamic rigid body to this monkey
		//	RigidBody::Sptr physics = LargeEnemy->Add<RigidBody>(RigidBodyType::Dynamic);
		//	ICollider::Sptr box = BoxCollider::Create();
		//	box->SetScale(glm::vec3(2.0f));
		//	box->SetPosition(glm::vec3(0.0f, 0.0f, 1.0f));
		//	physics->AddCollider(box);			
		//}
		//
		//GameObject::Sptr FastEnemy = scene->CreateGameObject("FastEnemy");
		//{
		//	// Set and rotation position in the scene
		//	FastEnemy->SetPostion(glm::vec3(10.0f, 1.0f, 10.0f));

		//	// Add a render component
		//	RenderComponent::Sptr renderer = FastEnemy->Add<RenderComponent>();
		//	renderer->SetMesh(FastEnemyMesh);
		//	renderer->SetMaterial(FastEnemyMaterial);

		//	//LargeEnemy->Add<EnemyBehaviour>();

		//	// This is an example of attaching a component and setting some parameters
		//	//RotatingBehaviour::Sptr behaviour = monkey2->Add<RotatingBehaviour>();
		//	//behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, -90.0f);

		//	// Add a dynamic rigid body to this monkey
		//	RigidBody::Sptr physics = FastEnemy->Add<RigidBody>(RigidBodyType::Dynamic);
		//	ICollider::Sptr box = BoxCollider::Create();
		//	box->SetScale(glm::vec3(2.0f));
		//	box->SetPosition(glm::vec3(0.0f, 0.0f, 1.0f));
		//	physics->AddCollider(box);
		//}

		//GameObject::Sptr NormalEnemy = scene->CreateGameObject("NormalEnemy");
		//{
		//	// Set and rotation position in the scene
		//	NormalEnemy->SetPostion(glm::vec3(20.0f, 1.0f, 10.0f));

		//	// Add a render component
		//	RenderComponent::Sptr renderer = NormalEnemy->Add<RenderComponent>();
		//	renderer->SetMesh(NormalEnemyMesh);
		//	renderer->SetMaterial(NormalEnemyMaterial);

		//	//LargeEnemy->Add<EnemyBehaviour>();

		//	// This is an example of attaching a component and setting some parameters
		//	//RotatingBehaviour::Sptr behaviour = monkey2->Add<RotatingBehaviour>();
		//	//behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, -90.0f);

		//	// Add a dynamic rigid body to this monkey
		//	RigidBody::Sptr physics = NormalEnemy->Add<RigidBody>(RigidBodyType::Dynamic);
		//	ICollider::Sptr box = BoxCollider::Create();
		//	box->SetScale(glm::vec3(2.0f));
		//	box->SetPosition(glm::vec3(0.0f, 0.0f, 1.0f));
		//	physics->AddCollider(box);
		//}

		// Call scene awake to start up all of our components
		scene->Window = window;
		scene->Awake();

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");
	}
}

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

		// End our ImGui window
		ImGui::End();

		VertexArrayObject::Unbind();

		lastFrame = thisFrame;
		ImGuiHelper::EndFrame();
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