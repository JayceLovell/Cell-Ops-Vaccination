#pragma once
#include "IComponent.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/EnemyBehaviour.h"
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

	float MaxHealth;
	int HealthInPercentage;
	std::string HealthUiName;

protected:
	float _health;
	RenderComponent::Sptr _renderer;
};

