#pragma once
#include <vector>
#include <GLM/detail/type_vec.hpp>
#include <main.cpp>
#include <Gameplay/GameObject.h>


class PathSampler
{
public:

	template<typename T>
	static T LERP(const T& p0, const T& p1, float t) {
		return (1.0f - t) * p0 + t * p1;
	}
	static const int NUM_SAMPLES;
	static const float SAMPLE_T;

	typedef std::vector<std::unique_ptr<GameObject>> KeypointSet;

	enum class PathMode
	{
		LERP,
	};

	PathMode m_mode;


private:

	std::vector<glm::vec3> m_samples;

protected:

	GameObject* m_owner;
	PathSampler* m_pathSource;
	Material* m_mat;


	

};

