#include "TargetController.h"
#include "Utils/ResourceManager/ResourceManager.h"

TargetController::TargetController():
	IComponent(),
	TargetNames(std::vector<std::string>()),
	TargetPositions(std::vector<glm::vec3>()),
	TargetMeshs(std::vector<Gameplay::MeshResource::Sptr>()),
	TargetMaterials(std::vector<Gameplay::Material::Sptr>()),
	TargetFrames(std::vector<Gameplay::MeshResource::Sptr>()),
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
		{ "TargetNames", TargetNames},
		{"TargetPositions",TargetPositions}
		//{"TargetMeshs",TargetMeshs}
		/*{ "TargetMeshs", TargetMeshs ? TargetMeshs->GetGUID().str() : "null"},
		{ "TargetMaterials", TargetMaterials},
		{ "TargetFrames", TargetFrames}*/
	};
}
TargetController::Sptr TargetController::FromJson(const nlohmann::json& blob)
{
	TargetController::Sptr result = std::make_shared<TargetController>();
	result->TargetNames = JsonGet(blob, "TargetNames", result->TargetNames);
	result->TargetPositions = JsonGet(blob, "TargetPositions", result->TargetPositions);
	/*result->TargetMeshs = JsonGet(blob, "TargetMeshs", result->TargetMeshs);
	result->TargetMeshs = JsonGet(blob, "TargetMeshs", result->TargetMeshs);
	result->TargetMaterials = JsonGet(blob, "TargetMaterials", result->TargetMaterials);
	result->TargetNames = JsonGet(blob, "TargetNames", result->TargetNames);*/
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
			Target->SetPostion(TargetPositions[i]);;
			
			/*Target->Add<ParticleSystem>();
			Target->Get<ParticleSystem>()->AddEmitter(Target->GetPosition(), glm::vec3(0.0f, 0.0f, 0.0f), 1.0f, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));*/
			

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
