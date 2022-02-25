#include "TargetController.h"

TargetController::TargetController():
	IComponent(),
	TargetNames(std::vector<std::string>()),
	TargetMesh(std::vector<Gameplay::MeshResource::Sptr>()),
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
		while (_isNotSafe) {
			float x = (float)(rand() % 100 + (-50));
			float y = (float)(rand() % 100 + (-50));
			float z = (float)(rand() % 100 + (-50));
			

			/// <summary>
			/// Check if near any other Targets
			/// </summary>
			if (_occupiedPositions.size() != 0) {
				for (auto check : _occupiedPositions) {
					if (!(_inRange(-abs(check.x), abs(check.x), x))) {
						if (!(_inRange(-abs(check.y), abs(check.y), y))) {
							if (!(_inRange(-abs(check.z), abs(check.z), z))) {
								_isNotSafe = false;;
							}
							else {
								_isNotSafe = true;
							}
						}
						else {
							_isNotSafe = true;
						}
					}
					else {
						_isNotSafe = true;
					}
				}
			}
			else {
				_isNotSafe = false;
			}
			_targetPosition=glm::vec3(x, y, z);
		}
		Gameplay::GameObject::Sptr Target = GetGameObject()->GetScene()->CreateGameObject(TargetNames[i]);
		{
			_occupiedPositions.push_back(_targetPosition);
			Target->SetPostion(_targetPosition);;
			

			// Add a render component
			RenderComponent::Sptr renderer = Target->Add<RenderComponent>();
			renderer->SetMesh(TargetMesh[i]);
			renderer->SetMaterial(TargetMaterials[i]);

			Gameplay::Physics::TriggerVolume::Sptr volume = Target->Add<Gameplay::Physics::TriggerVolume>();
			Gameplay::Physics::ConvexMeshCollider::Sptr collider = Gameplay::Physics::ConvexMeshCollider::Create();
			volume->AddCollider(collider);

			Target->Add<TargetBehaviour>();
			Target->Get<TargetBehaviour>()->TargetSetUp(100);

			GetGameObject()->GetScene()->Targets.push_back(Target);
			GetGameObject()->GetScene()->FindObjectByName("List Of Targets")->AddChild(Target);
		}
		_isNotSafe = true;
	}
}

bool TargetController::_inRange(float low, float high, float x)
{
	return ((x - high) * (x - low) <= 0);
}
