#include "EnemySpawnerBehaviour.h"

EnemySpawnerBehaviour::~EnemySpawnerBehaviour() = default;

EnemySpawnerBehaviour::EnemySpawnerBehaviour() :
	IComponent(),
	LargeEnemyMaterial(nullptr),
	LargeEnemyMesh(nullptr),
	_largeEnemySpeed(0.1f),
	NormalEnemyMaterial(nullptr),
	NormalEnemyMesh(nullptr),
	_normalEnemySpeed(0.3f),
	FastEnemyMaterial(nullptr),
	FastEnemyMesh(nullptr),
	_fastEnemySpeed(0.5f),
	_counter(0),
	_totalAmount(0),
	_isSpawning(false),
	_spawned(0)
{
}

void EnemySpawnerBehaviour::Update(float deltaTime)
{
	if (_isSpawning) {
		_counter++;
		if (_counter == 500) {
			_spawning();
		}
	}
}

nlohmann::json EnemySpawnerBehaviour::ToJson() const
{
	return {
		{"IsSpawning",_isSpawning},
		{"Delay Counter",_counter},
		{"Large Enemy Speed",_largeEnemySpeed},
		{"Normal Enemy Speed",_normalEnemySpeed},
		{"Fast Enemy Speed",_fastEnemySpeed},
		{"Amount of enemies",GetGameObject()->GetScene()->Enemies.size()},
		{"Total Spawning",_totalAmount},
		{"Spawned",_spawned}
	};

}

EnemySpawnerBehaviour::Sptr EnemySpawnerBehaviour::FromJson(const nlohmann::json& blob)
{
	EnemySpawnerBehaviour::Sptr result = std::make_shared<EnemySpawnerBehaviour>();
	result->_isSpawning = blob["IsSpawning"];
	result->_counter = blob["Delay Counter"];
	result->_largeEnemySpeed = blob["Large Enemy Speed"];
	result->_normalEnemySpeed = blob["Normal Enemy Speed"];
	result->_fastEnemySpeed = blob["Fast Enemy Speed"];
	result->_totalAmount = blob["Total Spawning"];
	result->_spawned = blob["Spawned"];
	return result;
}

void EnemySpawnerBehaviour::RenderImGui()
{
	LABEL_LEFT(ImGui::Checkbox, "IsSpawning", &_isSpawning);
	LABEL_LEFT(ImGui::DragInt, "Delay Counter", &_counter, 1.0f);
	LABEL_LEFT(ImGui::DragFloat, "Large Enemy Speed", &_largeEnemySpeed, 1.0f);
	LABEL_LEFT(ImGui::DragFloat, "Normal Enemy Speed", &_normalEnemySpeed, 1.0f);
	LABEL_LEFT(ImGui::DragFloat, "Fast Enemy Speed", &_fastEnemySpeed, 1.0f);
	LABEL_LEFT(ImGui::DragInt, "Total Spawning", &_totalAmount, 1.0f);
	LABEL_LEFT(ImGui::DragInt, "Spawned", &_spawned, 1.0f);
}

void EnemySpawnerBehaviour::SpawnWave(int LargeAmount, int NormalAmount, int FastAmount)
{
	_spawned = 0;
	_isSpawning = true;
	_largeAmount = LargeAmount;
	_normalAmount = NormalAmount;
	_fastAmount = FastAmount;
	_totalAmount = LargeAmount + NormalAmount + FastAmount;
}

void EnemySpawnerBehaviour::IncreaseEnemySpeed()
{
	_largeEnemySpeed += 0.1f;
	_normalEnemySpeed += 0.3f;
	_fastEnemySpeed += 0.5f;
}

void EnemySpawnerBehaviour::_createLargeEnemy()
{
	std::string EnemyName = "Enemy ID:" + std::to_string(GetGameObject()->GetScene()->Enemies.size());
	Gameplay::GameObject::Sptr LargeEnemy = GetGameObject()->GetScene()->CreateGameObject(EnemyName);
	{
		// Add a render component
		RenderComponent::Sptr renderer = LargeEnemy->Add<RenderComponent>();
		renderer->SetMesh(LargeEnemyMesh);
		renderer->SetMaterial(LargeEnemyMaterial);

		// Add a dynamic rigid body to this Enemy
		Gameplay::Physics::RigidBody::Sptr physics = LargeEnemy->Add<Gameplay::Physics::RigidBody>(RigidBodyType::Dynamic);
		physics->SetMass(0.0f);
		Gameplay::Physics::BoxCollider::Sptr collider = Gameplay::Physics::BoxCollider::Create();
		collider->SetScale(glm::vec3(3.04f, 4.23f, 3.44f));
		collider->SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
		physics->AddCollider(collider);

		LargeEnemy->Add<EnemyBehaviour>();
		LargeEnemy->Get<EnemyBehaviour>()->EnemyType = "Large Enemy";
		LargeEnemy->Get<EnemyBehaviour>()->Health = 5;
		LargeEnemy->Get<EnemyBehaviour>()->Speed = _largeEnemySpeed;

		MorphAnimator::Sptr animation = LargeEnemy->Add<MorphAnimator>();

		animation->AddClip(LargeEnemyFrames, 0.7f, "Idle");

		animation->ActivateAnim("Idle");

		GetGameObject()->GetScene()->Enemies.push_back(LargeEnemy);
		//GetGameObject()->GetScene()->FindObjectByName("Enemies")->AddChild(LargeEnemy);
	}
}

void EnemySpawnerBehaviour::_createNormalEnemy()
{
	std::string EnemyName = "Enemy ID:" + std::to_string(GetGameObject()->GetScene()->Enemies.size());
	Gameplay::GameObject::Sptr NormalEnemy = GetGameObject()->GetScene()->CreateGameObject(EnemyName);
	{
		// Add a render component
		RenderComponent::Sptr renderer = NormalEnemy->Add<RenderComponent>();
		renderer->SetMesh(NormalEnemyMesh);
		renderer->SetMaterial(NormalEnemyMaterial);

		// Add a dynamic rigid body to this monkey
		Gameplay::Physics::RigidBody::Sptr physics = NormalEnemy->Add<Gameplay::Physics::RigidBody>(RigidBodyType::Dynamic);
		physics->SetMass(0.0f);
		Gameplay::Physics::BoxCollider::Sptr collider = Gameplay::Physics::BoxCollider::Create();
		collider->SetScale(glm::vec3(1.130f, 1.120f, 1.790f));
		collider->SetPosition(glm::vec3(0.0f, 0.9f, 0.1f));
		physics->AddCollider(collider);


		NormalEnemy->Add<EnemyBehaviour>();
		NormalEnemy->Get<EnemyBehaviour>()->EnemyType = "Normal Enemy";
		NormalEnemy->Get<EnemyBehaviour>()->Health = 3;
		NormalEnemy->Get<EnemyBehaviour>()->Speed = _normalEnemySpeed;

		MorphAnimator::Sptr animation = NormalEnemy->Add<MorphAnimator>();

		animation->AddClip(NormalEnemyFrames, 0.7f, "Idle");

		animation->ActivateAnim("Idle");

		GetGameObject()->GetScene()->Enemies.push_back(NormalEnemy);
		//GetGameObject()->GetScene()->FindObjectByName("Enemies")->AddChild(NormalEnemy);
	}
}

void EnemySpawnerBehaviour::_createFastEnemy()
{
	std::string EnemyName = "Enemy ID:" + std::to_string(GetGameObject()->GetScene()->Enemies.size());
	Gameplay::GameObject::Sptr FastEnemy = GetGameObject()->GetScene()->CreateGameObject(EnemyName);
	{
		FastEnemy->SetPostion(GetGameObject()->SelfRef()->GetPosition());

		ParticleSystem::Sptr Emitter = FastEnemy->Add<ParticleSystem>();
		Emitter->AddEmitter(glm::vec3(0.0f), glm::vec3(20.0f, -1.0f, 10.0f), 20.0f, glm::vec4(0.0f, 0.18f, 1.0f, 1.0f));

		// Add a render component
		RenderComponent::Sptr renderer = FastEnemy->Add<RenderComponent>();
		renderer->SetMesh(FastEnemyMesh);
		renderer->SetMaterial(FastEnemyMaterial);

		// Add a dynamic rigid body to this enemy
		Gameplay::Physics::RigidBody::Sptr physics = FastEnemy->Add<Gameplay::Physics::RigidBody>(RigidBodyType::Dynamic);
		physics->SetMass(0.0f);
		Gameplay::Physics::BoxCollider::Sptr collider = Gameplay::Physics::BoxCollider::Create();
		collider->SetScale(glm::vec3(1.130f, 1.120f, 1.790f));
		collider->SetPosition(glm::vec3(0.0f, 0.0f, 1.0f));
		physics->AddCollider(collider);


		FastEnemy->Add<EnemyBehaviour>();
		FastEnemy->Get<EnemyBehaviour>()->EnemyType = "Fast Enemy";
		FastEnemy->Get<EnemyBehaviour>()->Health = 1;
		FastEnemy->Get<EnemyBehaviour>()->Speed = _fastEnemySpeed;


		/*MorphAnimator::Sptr animation = FastEnemy->Add<MorphAnimator>();
		animation->AddClip(FastEnemyFrames, 0.7f, "Idle");
		animation->ActivateAnim("Idle");*/

		GetGameObject()->GetScene()->Enemies.push_back(FastEnemy);
		//GetGameObject()->GetScene()->FindObjectByName("Enemies")->AddChild(FastEnemy);
	}
}

void EnemySpawnerBehaviour::_spawning()
{
	if (_totalAmount > _spawned)
	{
		int a = rand()%3;
		switch (a) {
		case 0:
			if (_fastAmount > 0) {
				_createFastEnemy();
				_fastAmount -= 1;
				_spawned++;
			}
			break;
		case 1:
			if (_normalAmount > 0) {
				_createNormalEnemy();
				_normalAmount -= 1;
				_spawned++;
			}
			break;
		case 2:
			if (_largeAmount > 0) {
				_createLargeEnemy();
				_largeAmount -= 1;
				_spawned++;
			}
			break;
		default:
			break;
		}
		_counter = 0;
	}
	else
	{
		_counter = 0;
		_isSpawning = false;
	}
}
