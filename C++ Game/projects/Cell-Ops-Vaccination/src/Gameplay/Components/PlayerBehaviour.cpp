#include "PlayerBehaviour.h"
#include <GLFW/glfw3.h>
#include "Utils/ImGuiHelper.h"
#include "Gameplay/Components/ComponentManager.h"



PlayerBehaviour::PlayerBehaviour() :
	IComponent()
{ }

PlayerBehaviour::~PlayerBehaviour() = default;

void PlayerBehaviour::Awake()
{
	
}

void PlayerBehaviour::RenderImGui() {
	
}

nlohmann::json PlayerBehaviour::ToJson() const {
	return {
		
	};
}


PlayerBehaviour::Sptr PlayerBehaviour::FromJson(const nlohmann::json & blob) {
	PlayerBehaviour::Sptr result = std::make_shared<PlayerBehaviour>();
	
	return result;
}

void PlayerBehaviour::OnEnteredTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger)
{
	LOG_INFO("Player Entered trigger: {}", trigger->GetGameObject()->Name);
}

void PlayerBehaviour::OnLeavingTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger)
{
	LOG_INFO("Player Left trigger: {}", trigger->GetGameObject()->Name);
}


void PlayerBehaviour::Update(float deltaTime) {
	GetGameObject()->SetRotation(GetGameObject()->GetScene()->FindObjectByName("Main Camera")->GetRotation());
	GetGameObject()->SetPostion(GetGameObject()->GetScene()->FindObjectByName("Main Camera")->GetPosition());
}