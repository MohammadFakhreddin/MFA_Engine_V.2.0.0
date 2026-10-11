#include "ShapeGenerator.hpp"

#include "BedrockAssert.hpp"

#include <ext/scalar_constants.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <utility>

namespace
{
    using Triangle = std::array<uint32_t, 3>;

    [[nodiscard]] uint64_t EdgeKey(uint32_t const first, uint32_t const second)
    {
        uint32_t const lower = std::min(first, second);
        uint32_t const upper = std::max(first, second);
        return (static_cast<uint64_t>(lower) << 32U) | static_cast<uint64_t>(upper);
    }
} // namespace

namespace ShapeGenerator
{

    //======================================================================================================================

    TexturedMesh Plane(float width, float height, int widthSegments, int heightSegments)
    {
        MFA_ASSERT(widthSegments > 0);
        MFA_ASSERT(heightSegments > 0);
        MFA_ASSERT(width > 0.0f);
        MFA_ASSERT(height > 0.0f);

        TexturedMesh mesh;
        auto &[vertices, indices, normals, textureCoordinates] = mesh;

        PlaneNoAlloc(width, height, widthSegments, heightSegments, vertices, indices, normals, textureCoordinates);

        return mesh;
    }

    //======================================================================================================================

    void PlaneNoAlloc(float width, float height, int widthSegments, int heightSegments, Vertices &vertices,
                      Indices &indices, Normals &normals, TextureCoordinates &textureCoordinates)
    {
        MFA_ASSERT(widthSegments > 0);
        MFA_ASSERT(heightSegments > 0);
        MFA_ASSERT(width > 0.0f);
        MFA_ASSERT(height > 0.0f);

        auto const vertexOffset = vertices.size();

        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        glm::vec2 bottomLeft(-halfWidth, -halfHeight);
        glm::vec2 topRight(halfWidth, halfHeight);

        for (int y = 0; y <= heightSegments; ++y)
        {
            for (int x = 0; x <= widthSegments; ++x)
            {
                float u = static_cast<float>(x) / static_cast<float>(widthSegments);
                float v = static_cast<float>(y) / static_cast<float>(heightSegments);

                float vX = glm::mix(bottomLeft.x, topRight.x, u);
                float vY = glm::mix(bottomLeft.y, topRight.y, v);

                vertices.emplace_back(vX, 0.0f, vY);
                normals.emplace_back(0.0f, -1.0f, 0.0f);
                textureCoordinates.emplace_back(u, v);
            }
        }

        for (int y = 0; y < heightSegments; ++y)
        {
            for (int x = 0; x < widthSegments; ++x)
            {
                int topLeft = y * (widthSegments + 1) + x + vertexOffset;
                int topRight = topLeft + 1;
                int bottomLeft = (y + 1) * (widthSegments + 1) + x + vertexOffset;
                int bottomRight = bottomLeft + 1;

                indices.emplace_back(bottomLeft);
                indices.emplace_back(topLeft);
                indices.emplace_back(topRight);

                indices.emplace_back(bottomLeft);
                indices.emplace_back(topRight);
                indices.emplace_back(bottomRight);
            }
        }
    }

    //======================================================================================================================

    Mesh Cylinder(float const radius, float const height, int const segments)
    {
        Vertices vertices{};
        Indices indices{};
        Normals normals{};

        float halfHeight = height / 2.0f;
        float angleStep = 2.0f * glm::pi<float>() / (float)segments;

        // Generate vertices and normals
        for (int i = 0; i < segments; ++i)
        {
            float angle = i * angleStep;
            float x = radius * cos(angle);
            float z = radius * sin(angle);

            // Bottom vertex
            vertices.emplace_back(x, -halfHeight, z);
            normals.emplace_back(x, 0.0f, z);

            // Top vertex
            vertices.emplace_back(x, halfHeight, z);
            normals.emplace_back(x, 0.0f, z);
        }

        // Add top and bottom center vertices
        vertices.emplace_back(0.0f, -halfHeight, 0.0f); // Bottom center
        normals.emplace_back(0.0f, -1.0f, 0.0f);

        vertices.emplace_back(0.0f, halfHeight, 0.0f); // Top center
        normals.emplace_back(0.0f, 1.0f, 0.0f);

        int bottomCenterIndex = segments * 2;
        int topCenterIndex = segments * 2 + 1;

        // Generate indices for the side faces
        for (int i = 0; i < segments; ++i)
        {
            int next = (i + 1) % segments;

            // Side face
            indices.emplace_back(i * 2);
            indices.emplace_back(next * 2);
            indices.emplace_back(i * 2 + 1);

            indices.emplace_back(i * 2 + 1);
            indices.emplace_back(next * 2);
            indices.emplace_back(next * 2 + 1);
        }

        // Generate indices for the bottom face
        for (int i = 0; i < segments; ++i)
        {
            int next = (i + 1) % segments;

            indices.emplace_back(bottomCenterIndex);
            indices.emplace_back(next * 2);
            indices.emplace_back(i * 2);
        }

        // Generate indices for the top face
        for (int i = 0; i < segments; ++i)
        {
            int next = (i + 1) % segments;

            indices.emplace_back(topCenterIndex);
            indices.emplace_back(i * 2 + 1);
            indices.emplace_back(next * 2 + 1);
        }

        return std::make_tuple(vertices, indices, normals);
    }

    //======================================================================================================================

    TexturedMesh Sphere(float const radius, int const subdivisions)
    {
        auto mesh = TexturedMesh{};
        auto &[positions, indices, normals, textureCoordinates] = mesh;

        SphereNoAlloc(radius, subdivisions, positions, indices, normals, textureCoordinates);

        return mesh;
    }


    //======================================================================================================================

    void SphereNoAlloc(float radius, int subdivisions, Vertices &positions, Indices &indices, Normals &normals,
                       TextureCoordinates &textureCoordinates)
    {
        constexpr float goldenRatio = 1.6180339887498948482f;
        constexpr int maxSubdivisionCount = 5;
        int const subdivisionCount = std::clamp(subdivisions, 0, maxSubdivisionCount);

        Vertices unitDirections{
            {-1.0f, goldenRatio, 0.0f},  {1.0f, goldenRatio, 0.0f},   {-1.0f, -goldenRatio, 0.0f},
            {1.0f, -goldenRatio, 0.0f},  {0.0f, -1.0f, goldenRatio},  {0.0f, 1.0f, goldenRatio},
            {0.0f, -1.0f, -goldenRatio}, {0.0f, 1.0f, -goldenRatio},  {goldenRatio, 0.0f, -1.0f},
            {goldenRatio, 0.0f, 1.0f},   {-goldenRatio, 0.0f, -1.0f}, {-goldenRatio, 0.0f, 1.0f},
        };
        for (auto &direction : unitDirections)
        {
            direction = glm::normalize(direction);
        }

        // Clockwise winding when viewed from outside, matching the renderer's front-face convention.
        std::vector<Triangle> triangles{
            {0, 5, 11},  {0, 1, 5},  {0, 7, 1},  {0, 10, 7}, {0, 11, 10}, {1, 9, 5}, {5, 4, 11},
            {11, 2, 10}, {10, 6, 7}, {7, 8, 1},  {3, 4, 9},  {3, 2, 4},   {3, 6, 2}, {3, 8, 6},
            {3, 9, 8},   {4, 5, 9},  {2, 11, 4}, {6, 10, 2}, {8, 7, 6},   {9, 1, 8},
        };

        for (int subdivision = 0; subdivision < subdivisionCount; ++subdivision)
        {
            std::unordered_map<uint64_t, uint32_t> midpointIndices{};
            midpointIndices.reserve(triangles.size() * 3 / 2);

            auto const midpointIndex = [&unitDirections, &midpointIndices](uint32_t const first,
                                                                           uint32_t const second) -> uint32_t
            {
                uint64_t const key = EdgeKey(first, second);
                if (auto const iterator = midpointIndices.find(key); iterator != midpointIndices.end())
                {
                    return iterator->second;
                }

                auto const index = static_cast<uint32_t>(unitDirections.size());
                unitDirections.emplace_back(glm::normalize(unitDirections[first] + unitDirections[second]));
                midpointIndices.emplace(key, index);
                return index;
            };

            std::vector<Triangle> subdividedTriangles{};
            subdividedTriangles.reserve(triangles.size() * 4);
            for (auto const &triangle : triangles)
            {
                uint32_t const firstToSecond = midpointIndex(triangle[0], triangle[1]);
                uint32_t const secondToThird = midpointIndex(triangle[1], triangle[2]);
                uint32_t const thirdToFirst = midpointIndex(triangle[2], triangle[0]);

                subdividedTriangles.push_back({triangle[0], firstToSecond, thirdToFirst});
                subdividedTriangles.push_back({triangle[1], secondToThird, firstToSecond});
                subdividedTriangles.push_back({triangle[2], thirdToFirst, secondToThird});
                subdividedTriangles.push_back({firstToSecond, secondToThird, thirdToFirst});
            }
            triangles = std::move(subdividedTriangles);
        }

        auto const sphericalUV = [](glm::vec3 const &direction) -> glm::vec2
        {
            float u = std::atan2(direction.z, direction.x) / (2.0f * glm::pi<float>());
            if (u < 0.0f)
            {
                u += 1.0f;
            }
            float const v = std::acos(std::clamp(direction.y, -1.0f, 1.0f)) / glm::pi<float>();
            return {u, v};
        };

        positions.reserve(positions.size() + unitDirections.size());
        normals.reserve(normals.size() + unitDirections.size());
        textureCoordinates.reserve(textureCoordinates.size() + unitDirections.size());
        indices.reserve(indices.size() + triangles.size() * 3);

        // A source vertex normally has one final vertex, plus an optional copy on the wrapped side of the UV seam.
        std::unordered_map<uint64_t, uint32_t> vertexVariants{};
        vertexVariants.reserve(unitDirections.size());

        auto const appendVertex = [&positions, &normals, &textureCoordinates, &unitDirections,
                                   radius](uint32_t const sourceIndex, glm::vec2 const uv) -> uint32_t
        {
            auto const index = static_cast<uint32_t>(positions.size());
            glm::vec3 const normal = unitDirections[sourceIndex];
            positions.emplace_back(normal * radius);
            normals.emplace_back(normal);
            textureCoordinates.emplace_back(uv);
            return index;
        };

        constexpr float poleEpsilon = 1.0e-6f;
        for (auto const &triangle : triangles)
        {
            std::array<glm::vec2, 3> triangleUVs{
                sphericalUV(unitDirections[triangle[0]]),
                sphericalUV(unitDirections[triangle[1]]),
                sphericalUV(unitDirections[triangle[2]]),
            };
            std::array<bool, 3> isPole{};
            for (size_t corner = 0; corner < triangle.size(); ++corner)
            {
                glm::vec3 const direction = unitDirections[triangle[corner]];
                isPole[corner] = std::abs(direction.x) < poleEpsilon && std::abs(direction.z) < poleEpsilon;
            }

            float minimumU = 1.0f;
            float maximumU = 0.0f;
            for (size_t corner = 0; corner < triangle.size(); ++corner)
            {
                if (isPole[corner] == false)
                {
                    minimumU = std::min(minimumU, triangleUVs[corner].x);
                    maximumU = std::max(maximumU, triangleUVs[corner].x);
                }
            }
            bool const crossesSeam = maximumU - minimumU > 0.5f;
            if (crossesSeam)
            {
                for (size_t corner = 0; corner < triangle.size(); ++corner)
                {
                    if (isPole[corner] == false && triangleUVs[corner].x < 0.5f)
                    {
                        triangleUVs[corner].x += 1.0f;
                    }
                }
            }

            for (size_t poleCorner = 0; poleCorner < triangle.size(); ++poleCorner)
            {
                if (isPole[poleCorner])
                {
                    size_t const nextCorner = (poleCorner + 1) % triangle.size();
                    size_t const previousCorner = (poleCorner + 2) % triangle.size();
                    triangleUVs[poleCorner].x = (triangleUVs[nextCorner].x + triangleUVs[previousCorner].x) * 0.5f;
                }
            }

            for (size_t corner = 0; corner < triangle.size(); ++corner)
            {
                uint32_t const sourceIndex = triangle[corner];
                if (isPole[corner])
                {
                    indices.emplace_back(appendVertex(sourceIndex, triangleUVs[corner]));
                    continue;
                }

                bool const wrapped = triangleUVs[corner].x >= 1.0f;
                uint64_t const variantKey = (static_cast<uint64_t>(sourceIndex) << 1U) | static_cast<uint64_t>(wrapped);
                if (auto const iterator = vertexVariants.find(variantKey); iterator != vertexVariants.end())
                {
                    indices.emplace_back(iterator->second);
                    continue;
                }

                uint32_t const index = appendVertex(sourceIndex, triangleUVs[corner]);
                vertexVariants.emplace(variantKey, index);
                indices.emplace_back(index);
            }
        }
    }

    //======================================================================================================================

    Mesh Quad()
    {
        Vertices vertices{};
        Indices indices{};
        Normals normals{};

        vertices.emplace_back(-1.0f, -1.0f, 0.0f);
        vertices.emplace_back(+1.0f, -1.0f, 0.0f);
        vertices.emplace_back(-1.0f, +1.0f, 0.0f);
        vertices.emplace_back(+1.0f, +1.0f, 0.0f);

        indices.emplace_back(0);
        indices.emplace_back(1);
        indices.emplace_back(2);
        indices.emplace_back(2);
        indices.emplace_back(1);
        indices.emplace_back(3);

        normals.emplace_back(0.0f, 0.0f, -1.0f);
        normals.emplace_back(0.0f, 0.0f, -1.0f);
        normals.emplace_back(0.0f, 0.0f, -1.0f);
        normals.emplace_back(0.0f, 0.0f, -1.0f);

        return std::make_tuple(vertices, indices, normals);
    }

    //======================================================================================================================

} // namespace ShapeGenerator
