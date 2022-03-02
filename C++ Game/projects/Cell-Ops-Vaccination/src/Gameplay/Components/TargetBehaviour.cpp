#include "TargetBehaviour.h"

TargetBehaviour::TargetBehaviour() :
	IComponent(),
	_health(0.0f),
	HealthInPercentage(0),
	_maxHealth(0.0f),
	HealthUiName("")
{
}

TargetBehaviour::~TargetBehaviour() = default;

void TargetBehaviour::Update(float deltaTime)
{
	HealthInPercentage = (_health * 100) / _maxHealth;
}
void TargetBehaviour::OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body)
{
	if (body->GetGameObject()->Name.find("Enemy") != std::string::npos) {
		std::string EnemyType = body->GetGameObject()->Get<EnemyBehaviour>()->EnemyType;
		if (EnemyType == "Fast Enemy") {
			_health = _health - 1;
		}
		else if (EnemyType == "Normal Enemy") {
			_health = _health - 3;
		}
		else if (EnemyType == "Large Enemy") {
			_health = _health - 5;
		}
		if (_health < 0) {
			GetGameObject()->GetScene()->DeleteTarget(GetGameObject()->SelfRef());
		}
	}
}

void TargetBehaviour::RenderImGui(){
	LABEL_LEFT(ImGui::DragFloat, "Health", &_health, 1.0f);
	LABEL_LEFT(ImGui::DragFloat, "MaxHealth", &_maxHealth, 1.0f);
}

nlohmann::json TargetBehaviour::ToJson() const
{
	return {
		{"health",_health},
		{"MaxHealth",_maxHealth}
	};
}

TargetBehaviour::Sptr TargetBehaviour::FromJson(const nlohmann::json& blob)
{
	TargetBehaviour::Sptr result = std::make_shared<TargetBehaviour>();
	result->_health = blob["Health"];
	result->_maxHealth = blob["MaxHealth"];
	return result;
}

void TargetBehaviour::Heal()
{
	_maxHealth += 10;
	_health = _maxHealth;
}

void TargetBehaviour::TargetSetUp(float MaxHealth)
{
	_maxHealth = MaxHealth;
	_health = MaxHealth;
	HealthInPercentage = (_health * 100) / MaxHealth;
}
