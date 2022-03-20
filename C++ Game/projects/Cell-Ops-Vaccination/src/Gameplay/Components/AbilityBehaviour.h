#pragma once
#include "IComponent.h"
#include "Utils/AudioEngine.h"

/// <summary>
/// Ability Class
/// </summary>
class AbilityBehaviour :public Gameplay::IComponent
{
public:
	typedef std::shared_ptr<AbilityBehaviour> Sptr;

	AbilityBehaviour();
	virtual ~AbilityBehaviour();

	virtual void Update(float deltaTime) override;
	virtual void RenderImGui() override;
	virtual nlohmann::json ToJson() const override;
	static AbilityBehaviour::Sptr FromJson(const nlohmann::json& blob);

	/// <summary>
	/// Abilities are 
	/// Johnson & Johnson
	/// Moderna
	/// Pfizer-BioNTech
	/// </summary>
	std::string PlayersAbilityChoice;

private:
	float _coolDownTimer;
};

