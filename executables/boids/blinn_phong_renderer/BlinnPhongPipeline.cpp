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
	: _params(params)
{
	mRenderPass = renderPass;
	mDescriptorPool = RB::CreateDescriptorPool(
		LogicalDevice::GetVkDevice(),
		_params.maxSets
	);

	CreatePerRenderDescriptorSetLayout();
	CreatePipeline();
}

//----------------------------------------------------------------------------------------------------------------------

BlinnPhongPipeline::~BlinnPhongPipeline()
{
	mPipeline = nullptr;
	mPerPipelineDescriptorLayout = nullptr;
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

// void BlinnPhongPipeline::SetPushConstants(RT::CommandRecordState& recordState, PushConstants pushConstants) const
// {
// 	RB::PushConstants(
// 		recordState,
// 		mPipeline->pipelineLayout,
// 		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
// 		0,
// 		Alias(pushConstants)
// 	);
// }

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongPipeline::CreatePerRenderDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings{};

	// ViewProjection
	VkDescriptorSetLayoutBinding modelViewProjectionBinding{
		.binding = static_cast<uint32_t>(bindings.size()),
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	};
	bindings.emplace_back(modelViewProjectionBinding);

    // Light source
    VkDescriptorSetLayoutBinding const lightSourceBinding{
        .binding = static_cast<uint32_t>(bindings.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    bindings.emplace_back(lightSourceBinding);

	mPerPipelineDescriptorLayout = RB::CreateDescriptorSetLayout(
		LogicalDevice::GetVkDevice(),
		static_cast<uint8_t>(bindings.size()),
		bindings.data()
	);
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongPipeline::CreatePipeline()
{
	// Vertex shader
	{
		bool success = Importer::CompileShaderToSPV(
			Path::Get("shaders/shape_pipeline/BlinnPhongPipeline.vert.hlsl"),
			Path::Get("shaders/shape_pipeline/BlinnPhongPipeline.vert.spv"),
			"vert"
		);
		MFA_ASSERT(success == true);
	}
	auto cpuVertexShader = Importer::ShaderFromSPV(
		Path::Get("shaders/shape_pipeline/BlinnPhongPipeline.vert.spv"),
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
			Path::Get("shaders/shape_pipeline/BlinnPhongPipeline.frag.hlsl"),
			Path::Get("shaders/shape_pipeline/BlinnPhongPipeline.frag.spv"),
			"frag"
		);
		MFA_ASSERT(success == true);
	}
	auto cpuFragmentShader = Importer::ShaderFromSPV(
		Path::Get("shaders/shape_pipeline/BlinnPhongPipeline.frag.spv"),
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
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, position),
	});
	// Normal
	inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
		.location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, normal),
	});
	// Instance
	for (int i = 0; i < 4; ++i)
	{
	    inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
            .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
            .binding = 1,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = (uint32_t)offsetof(Instance, model) + (uint32_t)(i * sizeof(glm::vec4))
        });
	}
	// Color
	inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
	    .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
        .binding = 1,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(Instance, color)
	});
    // Specular strength
	inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
        .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
        .binding = 1,
        .format = VK_FORMAT_R32_SFLOAT,
        .offset = offsetof(Instance, specularStrength)
    });
    // Shininess
	inputAttributeDescriptions.emplace_back(VkVertexInputAttributeDescription{
        .location = static_cast<uint32_t>(inputAttributeDescriptions.size()),
        .binding = 1,
        .format = VK_FORMAT_R32_SINT,
        .offset = offsetof(Instance, shininess)
    });

	RB::CreateGraphicPipelineOptions pipelineOptions{};
	pipelineOptions.useStaticViewportAndScissor = false;
	pipelineOptions.primitiveTopology = _params.topology;
	pipelineOptions.rasterizationSamples = LogicalDevice::GetMaxSampleCount();
	pipelineOptions.cullMode = _params.cullModeFlags;
	pipelineOptions.colorBlendAttachments.blendEnable = VK_TRUE;
	pipelineOptions.polygonMode = _params.polygonMode;

	// pipeline layout
	std::vector<VkPushConstantRange> const pushConstantRanges{
		// VkPushConstantRange {
		// 	.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
		// 	.offset = 0,
		// 	.size = sizeof(PushConstants),
		// }
	};

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{mPerPipelineDescriptorLayout->descriptorSetLayout};

	const auto pipelineLayout = RB::CreatePipelineLayout(
		LogicalDevice::GetVkDevice(),
		static_cast<uint32_t>(descriptorSetLayouts.size()),
		descriptorSetLayouts.data(),
		static_cast<uint32_t>(pushConstantRanges.size()),
		pushConstantRanges.data()
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

RT::DescriptorSetGroup BlinnPhongPipeline::CreatePerRenderDescriptorSets(
	const RT::BufferGroup & viewProjectionBuffer,
    const RT::BufferGroup & lightSourceBuffer
) const
{
	auto const maxFramesPerFlight = LogicalDevice::GetMaxFramePerFlight();
	auto descriptorSetGroup = RB::CreateDescriptorSet(
		LogicalDevice::GetVkDevice(),
		mDescriptorPool->descriptorPool,
		mPerPipelineDescriptorLayout->descriptorSetLayout,
		maxFramesPerFlight
	);

	for (uint32_t frameIndex = 0; frameIndex < maxFramesPerFlight; ++frameIndex)
	{

		auto const& descriptorSet = descriptorSetGroup.descriptorSets[frameIndex];
		MFA_ASSERT(descriptorSet != VK_NULL_HANDLE);

		DescriptorSetSchema descriptorSetSchema{ descriptorSet };

		/////////////////////////////////////////////////////////////////
		// Vertex shader
		/////////////////////////////////////////////////////////////////

		// ViewProjectionTransform
		VkDescriptorBufferInfo viewProjBufferInfo{
			.buffer = viewProjectionBuffer.buffers[frameIndex]->buffer,
			.offset = 0,
			.range = viewProjectionBuffer.bufferSize,
		};
		descriptorSetSchema.AddUniformBuffer(&viewProjBufferInfo);

		// LightSourceBuffer
		VkDescriptorBufferInfo lightSourceBufferInfo{
			.buffer = lightSourceBuffer.buffers[frameIndex]->buffer,
			.offset = 0,
			.range = lightSourceBuffer.bufferSize
		};
		descriptorSetSchema.AddUniformBuffer(&lightSourceBufferInfo);
		descriptorSetSchema.UpdateDescriptorSets();
	}

	return descriptorSetGroup;
}

//----------------------------------------------------------------------------------------------------------------------

void BlinnPhongPipeline::Reload()
{
	MFA_LOG_DEBUG("Reloading shading pipeline");

	LogicalDevice::DeviceWaitIdle();
	CreatePipeline();
}

//----------------------------------------------------------------------------------------------------------------------
