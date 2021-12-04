#include "PlayerBehaviour.h"
#include <GLFW/glfw3.h>
#include "Utils/ImGuiHelper.h"
#include "Gameplay/Components/ComponentManager.h"
#include <Gameplay/Components/EnemyBehaviour.h>



PlayerBehaviour::PlayerBehaviour() :
	IComponent(),
	EnemiesKilled(0)
{ }

PlayerBehaviour::~PlayerBehaviour() = default;

void PlayerBehaviour::Awake()
{
	_window = GetGameObject()->GetScene()->Window;
}

void PlayerBehaviour::OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body)
{
	//LOG_INFO("Player Entered trigger: {}", body->GetGameObject()->Name);
	if (body->GetGameObject()->Name == "Enemy" ||
		body->GetGameObject()->Name == "FastEnemy" ||
		body->GetGameObject()->Name == "LargeEnemy") {
		//if (glfwGetKey(_window, GLFW_KEY_Q) || glfwGetKey(_window, GLFW_KEY_E)) {
			LOG_INFO("Enemy Take Damage");
			body->GetGameObject()->Get<EnemyBehaviour>()->TakeDamage();
		//}
	}
}
void PlayerBehaviour::RenderImGui() {
	LABEL_LEFT(ImGui::DragInt, "Enemies Killed", &EnemiesKilled, 1.0f);
}

nlohmann::json PlayerBehaviour::ToJson() const {
	return {
		{"Enemies Killed",EnemiesKilled},
	};
}


PlayerBehaviour::Sptr PlayerBehaviour::FromJson(const nlohmann::json & blob) {
	PlayerBehaviour::Sptr result = std::make_shared<PlayerBehaviour>();
	result->EnemiesKilled = blob["Enemies Killed"];
	return result;
}


void PlayerBehaviour::Update(float deltaTime) {
	GetGameObject()->SetRotation(GetGameObject()->GetScene()->FindObjectByName("Main Camera")->GetRotation());
	GetGameObject()->SetPostion(GetGameObject()->GetScene()->FindObjectByName("Main Camera")->GetPosition());
	EnemiesKilled = GetGameObject()->GetScene()->EnemiesKilled;
}


