#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <tuple>
#include <vector>

namespace ShapeGenerator
{
    using Vertices = std::vector<glm::vec3>;
    using Indices = std::vector<uint32_t>;
    using Normals = std::vector<glm::vec3>;
    using TextureCoordinates = std::vector<glm::vec2>;
    using Mesh = std::tuple<Vertices, Indices, Normals>;
    using TexturedMesh = std::tuple<Vertices, Indices, Normals, TextureCoordinates>;

    [[nodiscard]]
    Mesh Cylinder(float radius, float height, int segments);

    [[nodiscard]]
    TexturedMesh Sphere(float radius, int subdivisions);

    [[nodiscard]]
    Mesh Quad();

};
