#pragma once

#include <glm/glm.hpp>

#include <span>

namespace MeshUtils
{
    class AABB
    {
    public:
        AABB() = default;

        explicit AABB(glm::vec3 const &min, glm::vec3 const &max);

        [[nodiscard]]
        glm::vec3 const& GetMin() const noexcept;

        [[nodiscard]]
        glm::vec3 const& GetMax() const noexcept;

        [[nodiscard]]
        glm::vec3 GetCenter() const noexcept;

        [[nodiscard]]
        glm::vec3 GetSize() const noexcept;

        [[nodiscard]]
        bool Contains(glm::vec3 const &point) const noexcept;
    
    private:
        glm::vec3 mMin{};
        glm::vec3 mMax{};
        
    };

    [[nodiscard]]
    AABB CalculateAABB(std::span<glm::vec3> vertices);
}