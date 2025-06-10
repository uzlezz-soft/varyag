#pragma once

#include <array>
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>

struct AABB
{
	glm::vec3 min;
	glm::vec3 max;

	AABB() = default;
	AABB(glm::vec3 min, glm::vec3 max) : min(min), max(max) {}
	AABB(const glm::mat4& worldMatrix, glm::vec3 localMin, glm::vec3 localMax);

	AABB TransformToWorld(const glm::mat4& worldMatrix) const
	{
		return { worldMatrix, min, max };
	}
};

struct Frustum
{
	std::array<glm::vec4, 6> planes;

	Frustum() = default;
	Frustum(const glm::mat4& viewProjection);

	bool IsAABBInside(const AABB& aabb) const;
};