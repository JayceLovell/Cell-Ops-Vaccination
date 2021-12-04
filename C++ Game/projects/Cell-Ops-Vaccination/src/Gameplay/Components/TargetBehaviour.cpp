#include "TargetBehaviour.h"

TargetBehaviour::TargetBehaviour():
	IComponent(),
	_health(0.0f),
	MaxHealth(0.0f)
{
}

TargetBehaviour::~TargetBehaviour() = default;

void TargetBehaviour::Awake()
{
	_health = MaxHealth;
}

void TargetBehaviour::Update(float deltaTime)
{
}
void TargetBehaviour::OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body)
{
	if (body->GetGameObject()->Name == "Enemy") {
		_health = _health - 2;
	}
	else if (body->GetGameObject()->Name == "FastEnemy")
	{
		_health = _health - 1;
	}
	else if (body->GetGameObject()->Name == "LargeEnemy") {
		_health = _health - 4;
	}
	if (_health < 0) {
		GetGameObject()->GetScene()->DeleteTarget(GetGameObject()->SelfRef());
	}
}

void TargetBehaviour::RenderImGui(){
	LABEL_LEFT(ImGui::DragFloat, "Health", &_health, 1.0f);
	LABEL_LEFT(ImGui::DragFloat, "MaxHealth", &MaxHealth, 1.0f);
}

nlohmann::json TargetBehaviour::ToJson() const
{
	return {
		{"health",_health},
		{"MaxHealth",MaxHealth}
	};
}

TargetBehaviour::Sptr TargetBehaviour::FromJson(const nlohmann::json& blob)
{
	TargetBehaviour::Sptr result = std::make_shared<TargetBehaviour>();
	result->_health = blob["Health"];
	result->MaxHealth = blob["MaxHealth"];
	return result;
}
