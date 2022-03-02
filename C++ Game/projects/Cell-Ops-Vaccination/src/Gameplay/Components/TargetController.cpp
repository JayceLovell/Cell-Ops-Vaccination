#include "TargetController.h"

TargetController::TargetController():
	IComponent(),
	TargetNames(std::vector<std::string>()),
	TargetMeshs(std::vector<Gameplay::MeshResource::Sptr>()),
	TargetMaterials(std::vector<Gameplay::Material::Sptr>()),
	TargetFrames(std::vector<Gameplay::MeshResource::Sptr>()),
	_occupiedPositions(std::vector<glm::vec3>()),
	_isNotSafe(true),
	_targetPosition()
{
}

TargetController::~TargetController() = default;

void TargetController::Update(float deltaTime)
{
}
nlohmann::json TargetController::ToJson() const
{
	return {
	};
}
TargetController::Sptr TargetController::FromJson(const nlohmann::json& blob)
{
	TargetController::Sptr result = std::make_shared<TargetController>();
	return result;
}
void TargetController::RenderImGui()
{
}
void TargetController::Spawntargets()
{
	for (int i = 0; i < TargetNames.size(); i++) {
		Gameplay::GameObject::Sptr Target = GetGameObject()->GetScene()->CreateGameObject(TargetNames[i]);
		{
			//_occupiedPositions.push_back(_targetPosition);
			Target->SetPostion(TargetPositions[i]);;
			

			// Add a render component
			RenderComponent::Sptr renderer = Target->Add<RenderComponent>();
			renderer->SetMesh(TargetMeshs[i]);
			renderer->SetMaterial(TargetMaterials[i]);

			Gameplay::Physics::TriggerVolume::Sptr volume = Target->Add<Gameplay::Physics::TriggerVolume>();
			Gameplay::Physics::ConvexMeshCollider::Sptr collider = Gameplay::Physics::ConvexMeshCollider::Create();
			volume->AddCollider(collider);

			Target->Add<TargetBehaviour>();
			Target->Get<TargetBehaviour>()->TargetSetUp(100);

			GetGameObject()->GetScene()->Targets.push_back(Target);
		}
		_isNotSafe = true;
	}
}

bool TargetController::_inRange(float low, float high, float x)
{
	return ((x - high) * (x - low) <= 0);
}
