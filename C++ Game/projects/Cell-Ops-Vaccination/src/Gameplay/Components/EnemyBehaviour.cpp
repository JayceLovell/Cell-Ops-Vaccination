#include "EnemyBehaviour.h"
#include <GLFW/glfw3.h>
#include "Utils/ImGuiHelper.h"

// Templated LERP function
template<typename T>
T LERP(const T& p0, const T& p1, float t)
{
	return (1.0f - t) * p0 + t * p1;
}

void EnemyBehaviour::Awake()
{
	//_body = GetComponent<Gameplay::Physics::RigidBody>();
	/*if (_body == nullptr) {
		IsEnabled = false;
	}*/
	_health = _maxHealth;
	RespawnPosition = GetGameObject()->GetPosition();
	Target = GetGameObject()->GetScene()->FindTarget();
	GetGameObject()->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
	GetGameObject()->LookAt(Target.get()->GetPosition());
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
	Target(nullptr),
	RespawnPosition(glm::vec3(0.0f,0.0f,0.0f))
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
	lerpTimer += deltaTime * _speed;
	if (lerpTimer >= lerpTimerMax) {
		lerpTimer = 0;
	}
	float t;
	t = lerpTimer / lerpTimerMax;

	GetGameObject()->SetPostion(LERP(RespawnPosition, Target.get()->GetPosition(), t));
}

// After destroying target look for new one
void EnemyBehaviour::NewTarget()
{
	Target = GetGameObject()->GetScene()->FindTarget();
}

void EnemyBehaviour::TakeDamage()
{
	LOG_INFO("I {} Took Damage", EnemyType);
	_health = _health - 1;
	if (_health <= 0) {
		LOG_INFO("Killed {}", EnemyType);
		GetGameObject()->GetScene()->EnemiesKilled++;
		GetGameObject()->GetScene()->UpdateUI();
		Reset();
	}
}

void EnemyBehaviour::Reset()
{
	_health = _maxHealth;
	float x = (float)(rand() % 50 + (-25));
	float y = (float)(rand() % 50 + (-25));
	float z = (float)(rand() % 50 + (-25));
	RespawnPosition = glm::vec3(x, y, z);
	GetGameObject()->SetPostion(RespawnPosition);
	NewTarget();
	GetGameObject()->LookAt(Target.get()->GetPosition());
}
