#pragma once

#include "pipeline/IShadingPipeline.hpp"
#include "RenderTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
// TODO: Use spirv-reflect to make your life easier

namespace MFA
{
    class BlinnPhongPipeline : public MFA::IShadingPipeline
    {
    public:

        static constexpr uint32_t TextureCount = 8;

        struct Vertex
        {
            glm::vec3 position{};
            glm::vec3 normal{};
            glm::vec2 albedoUV{};
        };

        struct Instance
        {
            glm::mat4 model = glm::mat4(1.0f);
            int32_t material = 0;
        };

        struct Material
        {
            glm::vec4 albedo = glm::vec4(1.0f);

            float specularStrength = 1.0f;
            float shininess = 32.0f;
            int32_t albedoTexture = -1;
            int32_t enableLighting = 1;
        };

        static_assert(sizeof(Material) == 32);
        static_assert(offsetof(Material, specularStrength) == 16);
        static_assert(offsetof(Material, shininess) == 20);
        static_assert(offsetof(Material, albedoTexture) == 24);
        static_assert(offsetof(Material, enableLighting) == 28);

        struct Camera
        {
            glm::mat4 viewProjection{};

            glm::vec3 position;
            float placeholder;
        };
        static_assert(sizeof(Camera) % 16 == 0);
        static_assert(offsetof(Camera, position) == 64);
        static_assert(offsetof(Camera, placeholder) == 76);

        // Directional light for now
        struct LightSource
        {
            glm::vec3 direction{};
            float ambientStrength{};
            glm::vec3 color{};
            float placeholder0{};
        };
        static_assert(sizeof(LightSource) % 16 == 0);
        static_assert(offsetof(LightSource, ambientStrength) == 12);
        static_assert(offsetof(LightSource, color) == 16);
        static_assert(offsetof(LightSource, placeholder0) == 28);

        struct Params
        {
            int maxSets = 100;
            
            VkCullModeFlags cullModeFlags = VK_CULL_MODE_BACK_BIT;
            VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
            bool dynamicCullMode = false;
            
            // Vertex
            uint32_t positionOffset = offsetof(Vertex, position);
            uint32_t positionBinding = 0;
            uint32_t normalOffset = offsetof(Vertex, normal);
            uint32_t normalBinding = 0;
            uint32_t uvOffset = offsetof(Vertex, albedoUV);
            uint32_t uvBinding = 0;

            // Instance
            uint32_t modelOffset = offsetof(Instance, model);
            uint32_t modelBinding = 1;
            uint32_t materialOffset = offsetof(Instance, material);
            uint32_t materialBinding = 1;

            std::vector<VkVertexInputBindingDescription> const bindingDescriptions
            {
                VkVertexInputBindingDescription
                {
                    .binding = 0,
                    .stride = sizeof(Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
                },
                VkVertexInputBindingDescription
                {
                    .binding = 1,
                    .stride = sizeof(Instance),
                    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
                },
            };
        };

        explicit BlinnPhongPipeline(
            VkRenderPass renderPass,
            Params params
        );

        ~BlinnPhongPipeline();

        [[nodiscard]]
        bool IsBinded(MFA::RT::CommandRecordState const& recordState) const;

        void BindPipeline(MFA::RT::CommandRecordState& recordState) const;

        void Reload() override;

        [[nodiscard]]
        RT::DescriptorSetGroup CreateCameraDescriptorSets(const RT::BufferGroup & cameraBuffer) const;

        [[nodiscard]]
        RT::DescriptorSetGroup CreateLightBufferDescriptorSets(const RT::BufferGroup & lightSourceBuffer) const;

        [[nodiscard]]
        RT::DescriptorSetGroup CreateMaterialDescriptorSets(
            RT::BufferGroup const & materialBuffer,
            RT::SamplerGroup const & sampler,
            size_t textureCount,
            RT::GpuTexture const * textures
        ) const;

        void BindCameraDescriptorSet(const MFA::RT::CommandRecordState& recordState, RT::DescriptorSetGroup const & descriptorSet) const;

        void BindLightDescriptorSet(const RT::CommandRecordState& recordState, RT::DescriptorSetGroup const & descriptorSet) const;

        void BindMaterialDescriptorSet(const RT::CommandRecordState& recordState, RT::DescriptorSetGroup const & descriptorSet) const;

    private:

        void CreateDescriptorSetLayout();

        void CreatePipeline();

        std::shared_ptr<RT::DescriptorPool> mDescriptorPool{};

        std::shared_ptr<RT::DescriptorSetLayoutGroup> mCameraDescriptorLayout{};

        std::shared_ptr<RT::DescriptorSetLayoutGroup> mLightDescriptorLayout{};

        std::shared_ptr<RT::DescriptorSetLayoutGroup> mMaterialDescriptorLayout{};

        std::shared_ptr<RT::PipelineGroup> mPipeline{};

        VkRenderPass mRenderPass{};

        Params mParams{};
    };
}
