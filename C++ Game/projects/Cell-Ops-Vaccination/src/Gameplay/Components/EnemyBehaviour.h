#pragma once
#include "IComponent.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Physics/TriggerVolume.h"

class EnemyBehaviour :public Gameplay::IComponent
{
public:
	typedef std::shared_ptr<EnemyBehaviour> Sptr;

	EnemyBehaviour();
	virtual ~EnemyBehaviour();

	virtual void Awake() override;

public:
	virtual void RenderImGui() override;
	virtual void Update(float deltaTime) override;
	virtual void OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body) override;
	virtual void OnTriggerVolumeLeaving(const std::shared_ptr<Gameplay::Physics::RigidBody>& body) override;
	void TakeDamage();
	MAKE_TYPENAME(EnemyBehaviour);
	virtual nlohmann::json ToJson() const override;
	static EnemyBehaviour::Sptr FromJson(const nlohmann::json& blob);

protected:
	float _health;
	float _dmg;

	//Gameplay::Physics::RigidBody::Sptr _body;
public:
	float _maxHealth;
	float _speed;
	std::string EnemyType;
	Gameplay::GameObject::Sptr Target;

	// After destroying target look for new one
	void NewTarget();
};