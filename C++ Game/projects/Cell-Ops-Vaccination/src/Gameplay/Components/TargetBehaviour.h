#pragma once
#include "IComponent.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Components/RenderComponent.h"
#include <Utils/ImGuiHelper.h>
#include <Gameplay/Components/GUI/GuiText.h>
#include <Gameplay/Components/GUI/GuiPanel.h>

/// <summary>
/// Target Behaviour
/// </summary>
class TargetBehaviour :public Gameplay::IComponent
{
public:
	typedef std::shared_ptr<TargetBehaviour> Sptr;

	Texture2D::Sptr FullHp;
	Texture2D::Sptr NintyPercentHp;
	Texture2D::Sptr EightyPercentHp;
	Texture2D::Sptr SeventyPercentHp;
	Texture2D::Sptr SixtyPercentHp;
	Texture2D::Sptr HalfHp;
	Texture2D::Sptr FortyPercentHp;
	Texture2D::Sptr ThirtyPercentHp;
	Texture2D::Sptr TwentyPercentHp;
	Texture2D::Sptr TenPercentHp;
	Texture2D::Sptr NoHp;
	Gameplay::GameObject::Sptr HealthUI;

	TargetBehaviour();
	virtual ~TargetBehaviour();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;
	virtual void OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body) override;
	virtual void RenderImGui() override;
	virtual nlohmann::json ToJson() const override;
	static TargetBehaviour::Sptr FromJson(const nlohmann::json& blob);
	MAKE_TYPENAME(TargetBehaviour);
	void Heal();
	void UpdateUI();

	float MaxHealth;

protected:
	float _health;
	int _healthInPercentage;
	RenderComponent::Sptr _renderer;
};

