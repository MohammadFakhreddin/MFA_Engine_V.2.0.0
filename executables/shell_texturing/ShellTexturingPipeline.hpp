#pragma once

#include "RenderTypes.hpp"
#include "pipeline/IShadingPipeline.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>

namespace MFA
{
    class ShellTexturingPipeline final : public IShadingPipeline
    {
    public:
        struct Vertex
        {
            glm::vec3 position{};
            glm::vec3 normal{};
            glm::vec2 uv{};
            glm::vec3 center{};
        };

        struct alignas(16) Camera
        {
            glm::mat4 viewProjection{1.0f};
        };

        struct alignas(16) Shell
        {
            float displacement = 0.015f;
            float radius = 4.0f;
            int32_t count = 48;
            float placeholder0 = 0.0f;
        };

        struct alignas(16) Model
        {
            glm::mat4 model{1.0f};
        };

        struct alignas(16) Material
        {
            glm::vec4 albedo{0.18f, 0.48f, 0.95f, 1.0f};
            int32_t albedoTexture = -1;
            std::array<int32_t, 3> alignmentPadding{};
            glm::vec4 placeholder0{};
        };

        struct alignas(16) Light
        {
            glm::vec3 direction{0.35f, 0.8f, 0.45f};
            float ambientStrength = 0.25f;
            glm::vec3 color{1.0f};
            float heightAttenuation = 1.0f;
        };

        static_assert(sizeof(Camera) == 64);
        static_assert(sizeof(Shell) == 16);
        static_assert(offsetof(Shell, radius) == 4);
        static_assert(offsetof(Shell, count) == 8);
        static_assert(sizeof(Model) == 64);
        static_assert(sizeof(Material) == 48);
        static_assert(offsetof(Material, albedoTexture) == 16);
        static_assert(offsetof(Material, placeholder0) == 32);
        static_assert(sizeof(Light) == 32);
        static_assert(offsetof(Light, ambientStrength) == 12);
        static_assert(offsetof(Light, color) == 16);
        static_assert(offsetof(Light, heightAttenuation) == 28);

        explicit ShellTexturingPipeline(VkRenderPass renderPass);
        ~ShellTexturingPipeline();

        void BindPipeline(RT::CommandRecordState &recordState) const;
        void BindDescriptorSets(
            RT::CommandRecordState const &recordState,
            RT::DescriptorSetGroup const &camera,
            RT::DescriptorSetGroup const &shell,
            RT::DescriptorSetGroup const &model,
            RT::DescriptorSetGroup const &material,
            RT::DescriptorSetGroup const &albedoTexture,
            RT::DescriptorSetGroup const &light
        ) const;

        [[nodiscard]] RT::DescriptorSetGroup CreateCameraDescriptorSets(RT::BufferGroup const &buffer) const;
        [[nodiscard]] RT::DescriptorSetGroup CreateShellDescriptorSets(RT::BufferGroup const &buffer) const;
        [[nodiscard]] RT::DescriptorSetGroup CreateModelDescriptorSets(RT::BufferGroup const &buffer) const;
        [[nodiscard]] RT::DescriptorSetGroup CreateMaterialDescriptorSets(RT::BufferGroup const &buffer) const;
        [[nodiscard]] RT::DescriptorSetGroup CreateAlbedoTextureDescriptorSet(
            RT::SamplerGroup const &sampler,
            RT::GpuTexture const &texture
        ) const;
        [[nodiscard]] RT::DescriptorSetGroup CreateLightDescriptorSets(RT::BufferGroup const &buffer) const;

        void Reload() override;

    private:
        [[nodiscard]] RT::DescriptorSetGroup CreateUniformDescriptorSets(
            RT::BufferGroup const &buffer,
            RT::DescriptorSetLayoutGroup const &layout
        ) const;

        void CreateDescriptorSetLayouts();
        void CreatePipeline();

        VkRenderPass _renderPass = VK_NULL_HANDLE;
        std::shared_ptr<RT::DescriptorPool> _descriptorPool{};
        std::array<std::shared_ptr<RT::DescriptorSetLayoutGroup>, 6> _descriptorSetLayouts{};
        std::shared_ptr<RT::PipelineGroup> _pipeline{};
    };
}
