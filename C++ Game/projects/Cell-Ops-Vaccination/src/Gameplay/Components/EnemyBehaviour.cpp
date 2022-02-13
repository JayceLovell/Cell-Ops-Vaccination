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
	RespawnPosition = GetGameObject()->GetPosition();
	Target = GetGameObject()->GetScene()->FindTarget();
}
void EnemyBehaviour::RenderImGui() {
	LABEL_LEFT(ImGui::DragFloat, "Speed", &Speed, 1.0f);
	LABEL_LEFT(ImGui::DragFloat, "Health", &Health, 1.0f);
	LABEL_LEFT(ImGui::Text, "Enemy Type", &EnemyType,1.0f);
}

nlohmann::json EnemyBehaviour::ToJson() const {
	return {
		{"speed",Speed},
		{"Health",Health},
		{"EnemyType",EnemyType}
	};
}

EnemyBehaviour::EnemyBehaviour() :
	IComponent(),
	Speed(0.0f),
	Health(0.0f),
	EnemyType(""),
	Target(nullptr),
	RespawnPosition(glm::vec3(0.0f,0.0f,0.0f))
{}

EnemyBehaviour::~EnemyBehaviour() = default;

EnemyBehaviour::Sptr EnemyBehaviour::FromJson(const nlohmann::json & blob) {
	EnemyBehaviour::Sptr result = std::make_shared<EnemyBehaviour>();
	result->Speed = blob["Speed"];
	result->Health = blob["Health"];
	result->EnemyType = blob["EnemyType"];
	return result;
}


void EnemyBehaviour::Update(float deltaTime)
{
	lerpTimer += deltaTime * Speed;
	if (lerpTimer >= lerpTimerMax) {
		lerpTimer = 0;
	}
	float t;
	t = lerpTimer / lerpTimerMax;
	GetGameObject()->SetPostion(LERP(RespawnPosition, Target.get()->GetPosition(), t));
	GetGameObject()->LookAt(Target.get()->GetPosition());
}

// After destroying target look for new one
void EnemyBehaviour::NewTarget()
{
	Target = GetGameObject()->GetScene()->FindTarget();
}

void EnemyBehaviour::TakeDamage()
{
	LOG_INFO("I {} Took Damage", EnemyType);
	Health = Health - 1;
	if (Health <= 0) {
		LOG_INFO("Killed {}", EnemyType);
		GetGameObject()->GetScene()->EnemiesKilled++;
		GetGameObject()->GetScene()->DeleteEnemy(GetGameObject()->SelfRef());
		GetGameObject()->GetScene()->RemoveGameObject(GetGameObject()->SelfRef());
	}
}

