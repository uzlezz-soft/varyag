#include "frustum.h"

AABB::AABB(const glm::mat4& worldMatrix, glm::vec3 localMin, glm::vec3 localMax)
{
    glm::vec3 worldMin(FLT_MAX);
    glm::vec3 worldMax(-FLT_MAX);

    // Define 8 corners of the AABB
    glm::vec3 corners[8] = {
        {localMin.x, localMin.y, localMin.z},
        {localMax.x, localMin.y, localMin.z},
        {localMin.x, localMax.y, localMin.z},
        {localMax.x, localMax.y, localMin.z},
        {localMin.x, localMin.y, localMax.z},
        {localMax.x, localMin.y, localMax.z},
        {localMin.x, localMax.y, localMax.z},
        {localMax.x, localMax.y, localMax.z}
    };

    // Transform each corner and find new min/max
    for (int i = 0; i < 8; i++) {
        glm::vec4 transformed = worldMatrix * glm::vec4(corners[i], 1.0f);
        glm::vec3 worldPos = glm::vec3(transformed);

        worldMin = glm::min(worldMin, worldPos);
        worldMax = glm::max(worldMax, worldPos);
    }

    min = worldMin;
    max = worldMax;
}

Frustum::Frustum(const glm::mat4& vp)
{
    planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]); // Left
    planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]); // Right
    planes[2] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]); // Near
    planes[3] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]); // Far
    planes[4] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]); // Bottom (adjusted for Z-up)
    planes[5] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]); // Top (adjusted for Z-up)

    for (int i = 0; i < 6; i++)
    {
        planes[i] /= glm::length(glm::vec3(planes[i])); // Normalize plane
    }
}

bool Frustum::IsAABBInside(const AABB& aabb) const
{
    for (const auto& plane : planes)
    {
        glm::vec3 positiveVertex = aabb.min;
        if (plane.x >= 0) positiveVertex.x = aabb.max.x;
        if (plane.y >= 0) positiveVertex.y = aabb.max.y;
        if (plane.z >= 0) positiveVertex.z = aabb.max.z;

        if (glm::dot(glm::vec3(plane), positiveVertex) + plane.w < 0)
        {
            return false;
        }
    }
    return true;
}
