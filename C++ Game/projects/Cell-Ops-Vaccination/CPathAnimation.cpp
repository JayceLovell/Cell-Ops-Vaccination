#include "CPathAnimation.h"

CPathAnimation::CPathAnimation(GameObject& owner) 
{
	m_owner = &owner;

	m_segmentIndex = 0;
	m_segmentTimer = 0.0f;
	m_segmentIndex2 = 1;

	m_segmentTravelTime = 1.0f;
	m_mode = PathSampler::PathMode::LERP;
}

void CPathAnimation::SetMode(PathSampler::PathMode mode)
{
	m_mode = mode;
	m_segmentIndex = 0;
}

void CPathAnimation::Update(const PathSampler::KeypointSet& keypoints, float deltaTime) 
{
	m_segmentTimer = m_segmentTimer + deltaTime;

	if (m_segmentTimer >= 1.0f) {
		m_segmentTimer = 0.0f;
		// Handle switching segments each time t = 1
		m_segmentIndex++;
		if (m_segmentIndex >= keypoints.size()) {
			m_segmentIndex = 0;
		}
		m_segmentIndex2 = m_segmentIndex + 1;
		if (m_segmentIndex2 >= keypoints.size()) {
			m_segmentIndex2 = 0;
		}
	}

	if (keypoints.size() > 0) {
		m_owner->transform.m_pos.x = PathSampler::LERP(keypoints[m_segmentIndex]->transform.m_pos.x, keypoints[m_segmentIndex2]->transform.m_pos.x, m_segmentTimer);
		m_owner->transform.m_pos.y = PathSampler::LERP(keypoints[m_segmentIndex]->transform.m_pos.y, keypoints[m_segmentIndex2]->transform.m_pos.y, m_segmentTimer);
		m_owner->transform.m_pos.z = PathSampler::LERP(keypoints[m_segmentIndex]->transform.m_pos.z, keypoints[m_segmentIndex2]->transform.m_pos.z, m_segmentTimer);
	}
}