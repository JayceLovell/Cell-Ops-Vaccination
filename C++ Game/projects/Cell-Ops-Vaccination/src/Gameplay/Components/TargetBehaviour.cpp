#include "TargetBehaviour.h"

TargetBehaviour::TargetBehaviour() :
	IComponent(),
	_health(0.0f),
	_healthInPercentage(0),
	MaxHealth(0.0f),
	FullHp(nullptr),
	NintyPercentHp(nullptr),
	EightyPercentHp(nullptr),
	SeventyPercentHp(nullptr),
	SixtyPercentHp(nullptr),
	HalfHp(nullptr),
	FortyPercentHp(nullptr),
	ThirtyPercentHp(nullptr),
	TwentyPercentHp(nullptr),
	TenPercentHp(nullptr),
	NoHp(nullptr),
	HealthUI(nullptr)
{
}

TargetBehaviour::~TargetBehaviour() = default;

void TargetBehaviour::Awake()
{
	_health = MaxHealth;
	UpdateUI();
}

void TargetBehaviour::Update(float deltaTime)
{
}
void TargetBehaviour::OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body)
{
	if (body->GetGameObject()->Name == "Enemy") {
		_health = _health - 2;
		UpdateUI();
	}
	else if (body->GetGameObject()->Name == "FastEnemy")
	{
		_health = _health - 1;
		UpdateUI();
	}
	else if (body->GetGameObject()->Name == "LargeEnemy") {
		_health = _health - 4;
		UpdateUI();
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

void TargetBehaviour::Heal()
{
	_health = MaxHealth;
}

void TargetBehaviour::UpdateUI()
{
	_healthInPercentage = (_health * 100) / MaxHealth;
	if (_healthInPercentage <= 0)
		HealthUI->Get<GuiPanel>()->SetTexture(NoHp);
	else if (_healthInPercentage < 10)
		HealthUI->Get<GuiPanel>()->SetTexture(TenPercentHp);
	else if (_healthInPercentage < 20)
		HealthUI->Get<GuiPanel>()->SetTexture(TwentyPercentHp);
	else if (_healthInPercentage < 30)
		HealthUI->Get<GuiPanel>()->SetTexture(ThirtyPercentHp);
	else if (_healthInPercentage < 40)
		HealthUI->Get<GuiPanel>()->SetTexture(FortyPercentHp);
	else if (_healthInPercentage < 50)
		HealthUI->Get<GuiPanel>()->SetTexture(HalfHp);
	else if (_healthInPercentage < 60)
		HealthUI->Get<GuiPanel>()->SetTexture(SixtyPercentHp);
	else if (_healthInPercentage < 70)
		HealthUI->Get<GuiPanel>()->SetTexture(SeventyPercentHp);
	else if (_healthInPercentage < 80)
		HealthUI->Get<GuiPanel>()->SetTexture(EightyPercentHp);
	else if (_healthInPercentage < 90)
		HealthUI->Get<GuiPanel>()->SetTexture(NintyPercentHp);
	else if (_healthInPercentage < 100)
		HealthUI->Get<GuiPanel>()->SetTexture(FullHp);

	HealthUI->Get<GuiText>()->SetText(HealthUI->Name + " " + std::to_string(_healthInPercentage) + '%');
}
