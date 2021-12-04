#pragma once
#include "IComponent.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include <Utils/ImGuiHelper.h>

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

protected:
	float _health;
};

