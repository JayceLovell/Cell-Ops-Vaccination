#include "EnemyBehaviour.h"
#include <GLFW/glfw3.h>
#include "Utils/ImGuiHelper.h"

void EnemyBehaviour::Awake()
{
	//_body = GetComponent<Gameplay::Physics::RigidBody>();
	/*if (_body == nullptr) {
		IsEnabled = false;
	}*/
	_health = _maxHealth;
}
void EnemyBehaviour::RenderImGui() {
	LABEL_LEFT(ImGui::DragFloat, "Speed", &_speed, 1.0f);
	LABEL_LEFT(ImGui::DragFloat, "Health", &_health, 1.0f);
	LABEL_LEFT(ImGui::DragFloat, "MaxHealth", &_maxHealth, 1.0f);
}

nlohmann::json EnemyBehaviour::ToJson() const {
	return {
		{"speed",_speed},
		{"health",_health},
		{"maxHealth",_maxHealth},
		{"EnemyType",EnemyType}
	};
}

EnemyBehaviour::EnemyBehaviour() :
	IComponent(),
	_speed(0.0f),
	_health(0.0f),
	_maxHealth(0.0f),
	EnemyType(""),
	Target(nullptr)
{}

EnemyBehaviour::~EnemyBehaviour() = default;

EnemyBehaviour::Sptr EnemyBehaviour::FromJson(const nlohmann::json & blob) {
	EnemyBehaviour::Sptr result = std::make_shared<EnemyBehaviour>();
	result->_health = blob["Health"];
	result->_maxHealth = blob["MaxHealth"];
	result->EnemyType = blob["EnemyType"];
	return result;
}


void EnemyBehaviour::Update(float deltaTime)
{
	//GetGameObject()->LookAt();
	GetGameObject()->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
	//GetGameObject()->SetPostion(glm::vec3(10.0f, 0.0f, 0.0));
}
void EnemyBehaviour::OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body)
{
	LOG_INFO("Body has entered {} trigger volume: {}", GetGameObject()->Name, body->GetGameObject()->Name);
}
void EnemyBehaviour::OnTriggerVolumeLeaving(const std::shared_ptr<Gameplay::Physics::RigidBody>& body)
{
	LOG_INFO("Body has left {} trigger volume: {}", GetGameObject()->Name, body->GetGameObject()->Name);
}
//void EnemyBehaviour::OnEnteredTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger)
//{
//	LOG_INFO("Ouch {}", EnemyType);
//	TakeDamage();
//	//_body->ApplyImpulse(glm::vec3(0.0f, 0.0f, 1.0f));
//}
// After destroying target look for new one
void EnemyBehaviour::NewTarget()
{
	// TODO: Add your implementation code here.
}

//void EnemyBehaviour::OnLeavingTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger)
//{
//	LOG_INFO("Leaving {}", EnemyType);
//}

void EnemyBehaviour::TakeDamage()
{
	_health = _health - 1;
	if (_health <= 0) {
		GetGameObject()->GetScene()->RemoveGameObject(GetGameObject()->SelfRef());
	}
}