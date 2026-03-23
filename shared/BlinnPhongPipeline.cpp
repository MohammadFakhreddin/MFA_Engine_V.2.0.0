#include "BlinnPhongPipeline.hpp"

#include "DescriptorSetSchema.hpp"
#include "LogicalDevice.hpp"
#include "BedrockPath.hpp"
#include "ImportShader.hpp"

using namespace MFA;

//----------------------------------------------------------------------------------------------------------------------

BlinnPhongPipeline::BlinnPhongPipeline(
	VkRenderPass renderPass,
	Params params
)
	: mParams(params)
{
	mRenderPass = renderPass;
	mDescriptorPool = RB::CreateDescriptorPool(
		LogicalDevice::GetVkDevice(),
		mParams.maxSets
	);

	CreateDescriptorSetLayout();
	CreatePipeline();
}

//----------------------------------------------------------------------------------------------------------------------

BlinnPhongPipeline::~BlinnPhongPipeline()
{
	mPipeline = nullptr;
	mCameraDescriptorLayout = nullptr;
    mLightDescriptorLayout = nullptr;
	mDescriptorPool = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

bool BlinnPhongPipeline::IsBinded(RT::CommandRecordState const& recordState) const
{
	if (recordState.pipeline == mPipeline.get())
	{
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongPipeline::BindPipeline(RT::CommandRecordState& recordState) const
{
	if (IsBinded(recordState))
	{
		return;
	}

	RB::BindPipeline(recordState, *mPipeline);
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongPipeline::CreateDescriptorSetLayout()
{
    {// Camera descriptor layout
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        // ViewProjection
        VkDescriptorSetLayoutBinding modelViewProjectionBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
        };
        bindings.emplace_back(modelViewProjectionBinding);

        mCameraDescriptorLayout = RB::CreateDescriptorSetLayout(
            LogicalDevice::GetVkDevice(),
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }
    {// Light source
        std::vector<VkDescriptorSetLayoutBinding> bindings{};

        VkDescriptorSetLayoutBinding const lightSourceBinding{
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        };
        bindings.emplace_back(lightSourceBinding);

        mLightDescriptorLayout = RB::CreateDescriptorSetLayout(
            LogicalDevice::GetVkDevice(),
            static_cast<uint8_t>(bindings.size()),
            bindings.data()
        );
    }
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongPipeline::CreatePipeline()
{
	// Vertex shader
	{
		bool success = Importer::CompileShaderToSPV(
			Path::Get("shaders/blinn_phong_pipeline/blinn_phong_pipeline.vert"),
			Path::Get("shaders/blinn_phong_pipeline/blinn_phong_pipeline.vert.spv"),
			"vert"
		);
		MFA_ASSERT(success == true);
	}
	auto cpuVertexShader = Importer::ShaderFromSPV(
		Path::Get("shaders/blinn_phong_pipeline/blinn_phong_pipeline.vert.spv"),
		VK_SHADER_STAGE_VERTEX_BIT,
		"main"
	);
	auto gpuVertexShader = RB::CreateShader(
		LogicalDevice::GetVkDevice(),
		cpuVertexShader
	);

	// Fragment shader
	{
		bool success = Importer::CompileShaderToSPV(
			Path::Get("shaders/blinn_phong_pipeline/blinn_phong_pipeline.frag"),
			Path::Get("shaders/blinn_phong_pipeline/blinn_phong_pipeline.frag.spv"),
			"frag"
		);
		MFA_ASSERT(success == true);
	}
	auto cpuFragmentShader = Importer::ShaderFromSPV(
		Path::Get("shaders/blinn_phong_pipeline/blinn_phong_pipeline.frag.spv"),
		VK_SHADER_STAGE_FRAGMENT_BIT,
		"main"
	);
	auto gpuFragmentShader = RB::CreateShader(
		LogicalDevice::GetVkDevice(),
		cpuFragmentShader
	);

	std::vector<RT::GpuShader const*> shaders{ gpuVertexShader.get(), gpuFragmentShader.get() };

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

	std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions{};
    // Vertex
	// Position
	inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
		.location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
		.binding = mParams.positionBinding,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = mParams.positionOffset,
	});
	// Normal
	inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
		.location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
		.binding = mParams.normalBinding,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = mParams.normalOffset,
	});
	// Instance
	for (int i = 0; i < 4; ++i)
	{
	    inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = mParams.modelBinding,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = mParams.modelOffset + static_cast<uint32_t>(i * sizeof(glm::vec4))
        });
	}
	// Color
	inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
	    .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
        .binding = mParams.colorBinding,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = mParams.colorOffset
	});
    // Specular strength
	inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
        .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
        .binding = mParams.specularStrengthBinding,
        .format = VK_FORMAT_R32_SFLOAT,
        .offset = mParams.specularStrengthOffset
    });
    // Shininess
	inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
        .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
        .binding = mParams.shininessBinding,
        .format = VK_FORMAT_R32_SINT,
        .offset = mParams.shininessOffset
    });

	RB::CreateGraphicPipelineOptions pipelineOptions{};
	pipelineOptions.useStaticViewportAndScissor = false;
	pipelineOptions.primitiveTopology = mParams.topology;
	pipelineOptions.rasterizationSamples = LogicalDevice::GetMaxSampleCount();
	pipelineOptions.cullMode = mParams.cullModeFlags;
	pipelineOptions.colorBlendAttachments.blendEnable = VK_TRUE;
	pipelineOptions.polygonMode = mParams.polygonMode;

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{mCameraDescriptorLayout->descriptorSetLayout, mLightDescriptorLayout->descriptorSetLayout};

	const auto pipelineLayout = RB::CreatePipelineLayout(
		LogicalDevice::GetVkDevice(),
		static_cast<uint32_t>(descriptorSetLayouts.size()),
		descriptorSetLayouts.data(),
		0,
		nullptr
	);

	auto surfaceCapabilities = LogicalDevice::GetSurfaceCapabilities();

	mPipeline = RB::CreateGraphicPipeline(
		LogicalDevice::GetVkDevice(),
		static_cast<uint8_t>(shaders.size()),
		shaders.data(),
		bindingDescriptions.size(),
		bindingDescriptions.data(),
		static_cast<uint8_t>(inputAttributeDescriptions.size()),
		inputAttributeDescriptions.data(),
		surfaceCapabilities.currentExtent,
		mRenderPass,
		pipelineLayout,
		pipelineOptions
	);
}

//----------------------------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup BlinnPhongPipeline::CreateCameraDescriptorSets(const RT::BufferGroup &cameraBuffer) const
{
    auto descriptorSetGroup = RB::CreateDescriptorSet(
        LogicalDevice::GetVkDevice(),
        mDescriptorPool->descriptorPool,
        mCameraDescriptorLayout->descriptorSetLayout,
        cameraBuffer.buffers.size()
    );

    for (uint32_t bIdx = 0; bIdx < cameraBuffer.buffers.size(); ++bIdx)
    {

        auto const &descriptorSet = descriptorSetGroup.descriptorSets[bIdx];
        MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

        DescriptorSetSchema descriptorSetSchema{descriptorSet};

        // ViewProjectionTransform
        VkDescriptorBufferInfo viewProjBufferInfo{
            .buffer = cameraBuffer.buffers[bIdx]->buffer,
            .offset = 0,
            .range = cameraBuffer.bufferSize,
        };
        descriptorSetSchema.AddUniformBuffer(&viewProjBufferInfo);
        descriptorSetSchema.UpdateDescriptorSets();
    }

    return descriptorSetGroup;
}

//----------------------------------------------------------------------------------------------------------------------

RT::DescriptorSetGroup BlinnPhongPipeline::CreateLightBufferDescriptorSets(const RT::BufferGroup &lightSourceBuffer) const
{
    auto descriptorSetGroup =
        RB::CreateDescriptorSet(
            LogicalDevice::GetVkDevice(),
            mDescriptorPool->descriptorPool,
            mLightDescriptorLayout->descriptorSetLayout,
            lightSourceBuffer.buffers.size()
        );

    for (uint32_t bIdx = 0; bIdx < lightSourceBuffer.buffers.size(); ++bIdx)
    {

        auto const &descriptorSet = descriptorSetGroup.descriptorSets[bIdx];
        MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

        DescriptorSetSchema descriptorSetSchema{descriptorSet};

        /////////////////////////////////////////////////////////////////
        // Vertex shader
        /////////////////////////////////////////////////////////////////

        // LightSourceBuffer
        VkDescriptorBufferInfo lightSourceBufferInfo{.buffer = lightSourceBuffer.buffers[bIdx]->buffer,
                                                     .offset = 0,
                                                     .range = lightSourceBuffer.bufferSize};
        descriptorSetSchema.AddUniformBuffer(&lightSourceBufferInfo);
        descriptorSetSchema.UpdateDescriptorSets();
    }

    return descriptorSetGroup;
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongPipeline::BindCameraDescriptorSet(const MFA::RT::CommandRecordState &recordState,
                                                         RT::DescriptorSetGroup const &descriptorSet) const
{
    RB::AutoBindDescriptorSet(recordState, (RB::UpdateFrequency)0, descriptorSet);
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongPipeline::BindLightDescriptorSet(const RT::CommandRecordState &recordState,
                                                RT::DescriptorSetGroup const &descriptorSet) const
{
    RB::AutoBindDescriptorSet(recordState, (RB::UpdateFrequency)1, descriptorSet);
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongPipeline::Reload()
{
	MFA_LOG_DEBUG("Reloading shading pipeline");

	LogicalDevice::DeviceWaitIdle();
	CreatePipeline();
}

//----------------------------------------------------------------------------------------------------------------------
