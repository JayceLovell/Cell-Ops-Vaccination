#pragma once
#include "IComponent.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/TargetBehaviour.h"
#include <Gameplay/Components/MorphAnimator.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/MeshResource.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Scene.h"
#include <Utils/ImGuiHelper.h>

class TargetController :public Gameplay::IComponent
{
public:
	typedef std::shared_ptr<TargetController> Sptr;

	TargetController();
	virtual ~TargetController();
	virtual void Update(float deltaTime) override;
	MAKE_TYPENAME(TargetController);
	virtual nlohmann::json ToJson() const override;
	static TargetController::Sptr FromJson(const nlohmann::json& blob);
	virtual void RenderImGui() override;

	//Names of Targets
	std::vector<std::string> TargetNames;

	//Target Position
	std::vector<glm::vec3> TargetPositions;

	//Materials stored in a vector
	std::vector<Gameplay::Material::Sptr> TargetMaterials;

	//Meshes stored in a vector
	std::vector<Gameplay::MeshResource::Sptr> TargetMeshs;

	//Animation
	std::vector<Gameplay::MeshResource::Sptr> TargetFrames;

	/// <summary>
	/// Spawns Targets Into Game
	/// </summary>
	void Spawntargets();
private:
	std::vector<glm::vec3> _occupiedPositions;
	bool _isNotSafe;
	glm::vec3 _targetPosition;

	/// <summary>
	/// Check if number is in range
	/// </summary>
	/// <param name="low">Negative of number</param>
	/// <param name="high">Positive of number</param>
	/// <param name="x">Number we are checking</param>
	/// <returns></returns>
	bool _inRange(float low, float high, float x);
};