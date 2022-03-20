#include "AbilityBehaviour.h"

AbilityBehaviour::AbilityBehaviour():
    IComponent()
{
}

AbilityBehaviour::~AbilityBehaviour() = default;

void AbilityBehaviour::Update(float deltaTime)
{
}

void AbilityBehaviour::RenderImGui()
{
    ImGui::DragFloat, "Cool down on Ability", & _coolDownTimer);
}

nlohmann::json AbilityBehaviour::ToJson() const
{
    return {

    };
}

AbilityBehaviour::Sptr AbilityBehaviour::FromJson(const nlohmann::json& blob)
{
    AbilityBehaviour::Sptr result = std::make_shared<AbilityBehaviour>();

    return result;
}
