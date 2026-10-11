#include "MeshUtils.hpp"

namespace MeshUtils
{

    AABB::AABB(glm::vec3 const &min, glm::vec3 const &max)
        : mMin(min), mMax(max)
    {
    }

    glm::vec3 const& AABB::GetMin() const noexcept
    {
        return mMin;
    }

    glm::vec3 const& AABB::GetMax() const noexcept
    {
        return mMax;
    }

    glm::vec3 AABB::GetCenter() const noexcept
    {
        return (mMin + mMax) * 0.5f;
    }

    glm::vec3 AABB::GetSize() const noexcept
    {
        return mMax - mMin;
    }

    bool AABB::Contains(glm::vec3 const &point) const noexcept
    {
        return (point.x >= mMin.x && point.x <= mMax.x) &&
               (point.y >= mMin.y && point.y <= mMax.y) &&
               (point.z >= mMin.z && point.z <= mMax.z);
    }

}