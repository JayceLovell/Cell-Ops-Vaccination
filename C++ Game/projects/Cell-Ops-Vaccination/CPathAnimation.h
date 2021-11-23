#pragma once
#include <iostream>
#include "Gameplay/GameObject.h"
#include <Gameplay/Components/IComponent.h>
#include <main.cpp>
#include "PathSampler.h"


class CPathAnimation
{
public:

	float m_segmentTravelTime;

	CPathAnimation(GameObject& owner);
	~CPathAnimation() = default;

	void SetMode(PathSampler::PathMode mode);
	void Update(const PathSampler::KeypointSet& keypoints, float deltaTime);

private:

	GameObject* m_owner;
	float m_segmentTimer;
	size_t m_segmentIndex;
	size_t m_segmentIndex2;
	PathSampler::PathMode m_mode;
};

