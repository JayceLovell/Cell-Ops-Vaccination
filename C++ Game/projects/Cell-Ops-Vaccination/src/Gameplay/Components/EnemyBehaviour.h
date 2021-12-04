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
	void TakeDamage();
	void Reset();
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

	glm::vec3 RespawnPosition;

	float lerpTimer = 0;
	float lerpTimerMax = 10.0f;

	// After destroying target look for new one
	void NewTarget();
};