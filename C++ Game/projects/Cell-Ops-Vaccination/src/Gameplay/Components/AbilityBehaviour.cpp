#include "AbilityBehaviour.h"

AbilityBehaviour::~AbilityBehaviour() = default;

AbilityBehaviour::AbilityBehaviour() :
    IComponent(),
    _coolDownTimer(1000.0f),
    _abilityIndex(3),
    _abilityActiveCounter(0.0f),
    _isAbilityActive(false)
{}

void AbilityBehaviour::Update(float deltaTime)
{
    if (_isAbilityActive) {
        _abilityActiveCounter--;
        if (_abilityActiveCounter < 0) {
            switch (_abilityIndex)
            {
            case 1:
                //return "Johnson & Johnson";
                _johnsonJohnson();
                break;
            case 2:
               //return "Moderna";
                _moderna();
                break;
            case 3:
                //return "Pfizer-BioNTech";
                _pfizerBioNTech();
                break;
            default:
                break;
            }
        }
    }
    else 
        _coolDownTimer--;

    if (InputEngine::GetKeyState(GLFW_KEY_E) == ButtonState::Pressed) 
    {
        if (_coolDownTimer < 1.0f) {
            ///Do ability
            switch (_abilityIndex)
            {
            case 1:
                //Johnson & Johnson
                _johnsonJohnson();
                break;
            case 2:
                //Moderna
                _moderna();
                break;
            case 3:
                //Pfizer-BioNTech Movment Speed Boost
                _pfizerBioNTech();                
                break;
            default:
                break;
            }
        }
        else {
            //play sound that ability isnt ready
            audioEngine->playSoundByName("AbilityNotReady");
        }
    }
}

void AbilityBehaviour::RenderImGui()
{
    ImGui::DragFloat( "Cool down on Ability", & _coolDownTimer);
    ImGui::DragInt("Ability Index: ", &_abilityIndex);
}

nlohmann::json AbilityBehaviour::ToJson() const
{
    return {
        {"Ability Index",_abilityIndex},
    };
}

AbilityBehaviour::Sptr AbilityBehaviour::FromJson(const nlohmann::json& blob)
{
    AbilityBehaviour::Sptr result = std::make_shared<AbilityBehaviour>();

    return result;
}

void AbilityBehaviour::SetPlayersAbilityChoice(std::string Ability)
{
    if (Ability == "Johnson & Johnson") {
        _abilityIndex = 1;
    }
    else if (Ability == "Moderna") {
        _abilityIndex = 2;
    }
    else if (Ability == "Pfizer-BioNTech") {
        _abilityIndex = 3;
    }
}

std::string AbilityBehaviour::GetPlayersAbilityChoice()
{
    switch (_abilityIndex)
    {
    case 1:
        return "Johnson & Johnson";
        break;
    case 2:
        return "Moderna";
        break;
    case 3:
        return "Pfizer-BioNTech";
        break;
    default:
        return "No Ability";
        break;
    }
}

void AbilityBehaviour::_pfizerBioNTech()
{
    if (!GetGameObject()->GetScene()->MainCamera->GetComponent<SimpleCameraControl>()->isAbilityActive) {
        GetGameObject()->GetScene()->MainCamera->GetComponent<SimpleCameraControl>()->isAbilityActive = true;
        audioEngine->playSoundByName("AbilityPfizer-BioNTech");
        _abilityActiveCounter = 500.0f;
        _isAbilityActive = true;
    }
    else
    {
        GetGameObject()->GetScene()->MainCamera->GetComponent<SimpleCameraControl>()->isAbilityActive = false;
        _coolDownTimer = 1500.0f;
        _isAbilityActive = false;
    }
}

void AbilityBehaviour::_moderna()
{
    audioEngine->playSoundByName("AbilityModerna");
}

void AbilityBehaviour::_johnsonJohnson()
{
    audioEngine->playSoundByName("AbilityJohnson&Johnson");
}
