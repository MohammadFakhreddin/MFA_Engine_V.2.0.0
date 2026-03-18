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

        struct Vertex
        {
            glm::vec3 position{};
            glm::vec3 normal{};
        };

        struct Instance
        {
            glm::mat4 model = glm::mat4(1.0f);
            glm::vec4 color = glm::vec4(1.0f);
            float specularStrength = 1.0f;
            int shininess = 32;
        };

        struct Camera
        {
            glm::mat4 viewProjection{};

            glm::vec3 position;
            float placeholder;
        };

        // Directional light for now
        struct LightSource
        {
            glm::vec3 direction{};
            float ambientStrength{};
            glm::vec3 color{};
            float placeholder0{};
        };

        struct Params
        {
            int maxSets = 100;
            
            VkCullModeFlags cullModeFlags = VK_CULL_MODE_BACK_BIT;
            VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
            
            uint32_t positionOffset = offsetof(Vertex, position);
            uint32_t positionBinding = 0;
            uint32_t normalOffset = offsetof(Vertex, normal);
            uint32_t normalBinding = 0;
            uint32_t modelOffset = offsetof(Instance, model);
            uint32_t modelBinding = 1;
            uint32_t colorOffset = offsetof(Instance, color);
            uint32_t colorBinding = 1;
            uint32_t specularStrengthOffset = offsetof(Instance, specularStrength);
            uint32_t specularStrengthBinding = 1;
            uint32_t shininessOffset = offsetof(Instance, shininess);
            uint32_t shininessBinding = 1;
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
        MFA::RT::DescriptorSetGroup CreatePerRenderDescriptorSets(
            const MFA::RT::BufferGroup & viewProjectionBuffer,
            const MFA::RT::BufferGroup & lightSourceBuffer
        ) const;

    private:

        void CreatePerRenderDescriptorSetLayout();

        void CreatePipeline();

        std::shared_ptr<MFA::RT::DescriptorPool> mDescriptorPool{};

        std::shared_ptr<MFA::RT::DescriptorSetLayoutGroup> mPerPipelineDescriptorLayout{};

        std::shared_ptr<MFA::RT::PipelineGroup> mPipeline{};

        VkRenderPass mRenderPass{};

        Params mParams{};
    };
}
