/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Utilities for creating commonly used Vulkan objects
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "vkRefUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkObjUtil.hpp"
#include "vkTypeUtil.hpp"

#include "tcuVector.hpp"

#include "deSTLUtil.hpp"

namespace vk
{

Move<VkPipeline> makeComputePipeline(const DeviceInterface &vk, const VkDevice device,
                                     const VkPipelineLayout pipelineLayout, const VkPipelineCreateFlags pipelineFlags,
                                     const VkShaderModule shaderModule,
                                     const VkPipelineShaderStageCreateFlags shaderFlags,
                                     const VkSpecializationInfo *specializationInfo)
{
    const VkPipelineShaderStageCreateInfo pipelineShaderStageParams = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // VkStructureType sType;
        nullptr,                                             // const void* pNext;
        shaderFlags,                                         // VkPipelineShaderStageCreateFlags flags;
        VK_SHADER_STAGE_COMPUTE_BIT,                         // VkShaderStageFlagBits stage;
        shaderModule,                                        // VkShaderModule module;
        "main",                                              // const char* pName;
        specializationInfo,                                  // const VkSpecializationInfo* pSpecializationInfo;
    };
    const VkComputePipelineCreateInfo pipelineCreateInfo = {
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, // VkStructureType sType;
        nullptr,                                        // const void* pNext;
        pipelineFlags,                                  // VkPipelineCreateFlags flags;
        pipelineShaderStageParams,                      // VkPipelineShaderStageCreateInfo stage;
        pipelineLayout,                                 // VkPipelineLayout layout;
        DE_NULL,                                        // VkPipeline basePipelineHandle;
        0,                                              // int32_t basePipelineIndex;
    };
    return createComputePipeline(vk, device, DE_NULL, &pipelineCreateInfo);
}

Move<VkPipeline> makeGraphicsPipeline(
    const DeviceInterface &vk, const VkDevice device, const VkPipelineLayout pipelineLayout,
    const VkShaderModule vertexShaderModule, const VkShaderModule tessellationControlShaderModule,
    const VkShaderModule tessellationEvalShaderModule, const VkShaderModule geometryShaderModule,
    const VkShaderModule fragmentShaderModule, const VkRenderPass renderPass, const std::vector<VkViewport> &viewports,
    const std::vector<VkRect2D> &scissors, const VkPrimitiveTopology topology, const uint32_t subpass,
    const uint32_t patchControlPoints, const VkPipelineVertexInputStateCreateInfo *vertexInputStateCreateInfo,
    const VkPipelineRasterizationStateCreateInfo *rasterizationStateCreateInfo,
    const VkPipelineMultisampleStateCreateInfo *multisampleStateCreateInfo,
    const VkPipelineDepthStencilStateCreateInfo *depthStencilStateCreateInfo,
    const VkPipelineColorBlendStateCreateInfo *colorBlendStateCreateInfo,
    const VkPipelineDynamicStateCreateInfo *dynamicStateCreateInfo)
{
    const VkBool32 disableRasterization = (fragmentShaderModule == DE_NULL);
    const bool hasTessellation =
        (tessellationControlShaderModule != DE_NULL || tessellationEvalShaderModule != DE_NULL);

    VkPipelineShaderStageCreateInfo stageCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // VkStructureType                     sType
        DE_NULL,                                             // const void*                         pNext
        0u,                                                  // VkPipelineShaderStageCreateFlags    flags
        VK_SHADER_STAGE_VERTEX_BIT,                          // VkShaderStageFlagBits               stage
        DE_NULL,                                             // VkShaderModule                      module
        "main",                                              // const char*                         pName
        DE_NULL                                              // const VkSpecializationInfo*         pSpecializationInfo
    };

    std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageParams;

    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stageCreateInfo.module = vertexShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    if (tessellationControlShaderModule != DE_NULL)
    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        stageCreateInfo.module = tessellationControlShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    if (tessellationEvalShaderModule != DE_NULL)
    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        stageCreateInfo.module = tessellationEvalShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    if (geometryShaderModule != DE_NULL)
    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
        stageCreateInfo.module = geometryShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    if (fragmentShaderModule != DE_NULL)
    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stageCreateInfo.module = fragmentShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    const VkVertexInputBindingDescription vertexInputBindingDescription = {
        0u,                          // uint32_t             binding
        sizeof(tcu::Vec4),           // uint32_t             stride
        VK_VERTEX_INPUT_RATE_VERTEX, // VkVertexInputRate    inputRate
    };

    const VkVertexInputAttributeDescription vertexInputAttributeDescription = {
        0u,                            // uint32_t    location
        0u,                            // uint32_t    binding
        VK_FORMAT_R32G32B32A32_SFLOAT, // VkFormat    format
        0u                             // uint32_t    offset
    };

    const VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfoDefault = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // VkStructureType                             sType
        DE_NULL,                                                   // const void*                                 pNext
        (VkPipelineVertexInputStateCreateFlags)0,                  // VkPipelineVertexInputStateCreateFlags       flags
        1u,                              // uint32_t                                    vertexBindingDescriptionCount
        &vertexInputBindingDescription,  // const VkVertexInputBindingDescription*      pVertexBindingDescriptions
        1u,                              // uint32_t                                    vertexAttributeDescriptionCount
        &vertexInputAttributeDescription // const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, // VkStructureType                            sType
        DE_NULL,                                                     // const void*                                pNext
        0u,                                                          // VkPipelineInputAssemblyStateCreateFlags    flags
        topology, // VkPrimitiveTopology                        topology
        VK_FALSE  // VkBool32                                   primitiveRestartEnable
    };

    const VkPipelineTessellationStateCreateInfo tessStateCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, // VkStructureType                           sType
        DE_NULL,                                                   // const void*                               pNext
        0u,                                                        // VkPipelineTessellationStateCreateFlags    flags
        patchControlPoints // uint32_t                                  patchControlPoints
    };

    const VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, // VkStructureType                             sType
        DE_NULL,                                               // const void*                                 pNext
        (VkPipelineViewportStateCreateFlags)0,                 // VkPipelineViewportStateCreateFlags          flags
        viewports.empty() ? 1u :
                            (uint32_t)viewports.size(),     // uint32_t                                    viewportCount
        viewports.empty() ? DE_NULL : &viewports[0],        // const VkViewport*                           pViewports
        viewports.empty() ? 1u : (uint32_t)scissors.size(), // uint32_t                                    scissorCount
        scissors.empty() ? DE_NULL : &scissors[0]           // const VkRect2D*                             pScissors
    };

    const VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfoDefault = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // VkStructureType                            sType
        DE_NULL,                                                    // const void*                                pNext
        0u,                                                         // VkPipelineRasterizationStateCreateFlags    flags
        VK_FALSE,                        // VkBool32                                   depthClampEnable
        disableRasterization,            // VkBool32                                   rasterizerDiscardEnable
        VK_POLYGON_MODE_FILL,            // VkPolygonMode                              polygonMode
        VK_CULL_MODE_NONE,               // VkCullModeFlags                            cullMode
        VK_FRONT_FACE_COUNTER_CLOCKWISE, // VkFrontFace                                frontFace
        VK_FALSE,                        // VkBool32                                   depthBiasEnable
        0.0f,                            // float                                      depthBiasConstantFactor
        0.0f,                            // float                                      depthBiasClamp
        0.0f,                            // float                                      depthBiasSlopeFactor
        1.0f                             // float                                      lineWidth
    };

    const VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfoDefault = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, // VkStructureType                          sType
        DE_NULL,                                                  // const void*                              pNext
        0u,                                                       // VkPipelineMultisampleStateCreateFlags    flags
        VK_SAMPLE_COUNT_1_BIT, // VkSampleCountFlagBits                    rasterizationSamples
        VK_FALSE,              // VkBool32                                 sampleShadingEnable
        1.0f,                  // float                                    minSampleShading
        DE_NULL,               // const VkSampleMask*                      pSampleMask
        VK_FALSE,              // VkBool32                                 alphaToCoverageEnable
        VK_FALSE               // VkBool32                                 alphaToOneEnable
    };

    const VkStencilOpState stencilOpState = {
        VK_STENCIL_OP_KEEP,  // VkStencilOp    failOp
        VK_STENCIL_OP_KEEP,  // VkStencilOp    passOp
        VK_STENCIL_OP_KEEP,  // VkStencilOp    depthFailOp
        VK_COMPARE_OP_NEVER, // VkCompareOp    compareOp
        0,                   // uint32_t       compareMask
        0,                   // uint32_t       writeMask
        0                    // uint32_t       reference
    };

    const VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfoDefault = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, // VkStructureType                          sType
        DE_NULL,                                                    // const void*                              pNext
        0u,                                                         // VkPipelineDepthStencilStateCreateFlags   flags
        VK_FALSE,                    // VkBool32                                 depthTestEnable
        VK_FALSE,                    // VkBool32                                 depthWriteEnable
        VK_COMPARE_OP_LESS_OR_EQUAL, // VkCompareOp                              depthCompareOp
        VK_FALSE,                    // VkBool32                                 depthBoundsTestEnable
        VK_FALSE,                    // VkBool32                                 stencilTestEnable
        stencilOpState,              // VkStencilOpState                         front
        stencilOpState,              // VkStencilOpState                         back
        0.0f,                        // float                                    minDepthBounds
        1.0f,                        // float                                    maxDepthBounds
    };

    const VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
        VK_FALSE,                // VkBool32                 blendEnable
        VK_BLEND_FACTOR_ZERO,    // VkBlendFactor            srcColorBlendFactor
        VK_BLEND_FACTOR_ZERO,    // VkBlendFactor            dstColorBlendFactor
        VK_BLEND_OP_ADD,         // VkBlendOp                colorBlendOp
        VK_BLEND_FACTOR_ZERO,    // VkBlendFactor            srcAlphaBlendFactor
        VK_BLEND_FACTOR_ZERO,    // VkBlendFactor            dstAlphaBlendFactor
        VK_BLEND_OP_ADD,         // VkBlendOp                alphaBlendOp
        VK_COLOR_COMPONENT_R_BIT // VkColorComponentFlags    colorWriteMask
            | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    const VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfoDefault = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, // VkStructureType                               sType
        DE_NULL,                                                  // const void*                                   pNext
        0u,                                                       // VkPipelineColorBlendStateCreateFlags          flags
        VK_FALSE,                   // VkBool32                                      logicOpEnable
        VK_LOGIC_OP_CLEAR,          // VkLogicOp                                     logicOp
        1u,                         // uint32_t                                      attachmentCount
        &colorBlendAttachmentState, // const VkPipelineColorBlendAttachmentState*    pAttachments
        {0.0f, 0.0f, 0.0f, 0.0f}    // float                                         blendConstants[4]
    };

    std::vector<VkDynamicState> dynamicStates;

    if (viewports.empty())
        dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    if (scissors.empty())
        dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

    const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfoDefault = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, // VkStructureType                      sType
        DE_NULL,                                              // const void*                          pNext
        0u,                                                   // VkPipelineDynamicStateCreateFlags    flags
        (uint32_t)dynamicStates.size(),                       // uint32_t                             dynamicStateCount
        dynamicStates.empty() ? DE_NULL : &dynamicStates[0]   // const VkDynamicState*                pDynamicStates
    };

    const VkPipelineDynamicStateCreateInfo *dynamicStateCreateInfoDefaultPtr =
        dynamicStates.empty() ? DE_NULL : &dynamicStateCreateInfoDefault;

    const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, // VkStructureType                                  sType
        DE_NULL,                                         // const void*                                      pNext
        0u,                                              // VkPipelineCreateFlags                            flags
        (uint32_t)pipelineShaderStageParams.size(),      // uint32_t                                         stageCount
        &pipelineShaderStageParams[0],                   // const VkPipelineShaderStageCreateInfo*           pStages
        vertexInputStateCreateInfo ?
            vertexInputStateCreateInfo :
            &vertexInputStateCreateInfoDefault, // const VkPipelineVertexInputStateCreateInfo*      pVertexInputState
        &inputAssemblyStateCreateInfo,          // const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState
        hasTessellation ? &tessStateCreateInfo :
                          DE_NULL, // const VkPipelineTessellationStateCreateInfo*     pTessellationState
        &viewportStateCreateInfo,  // const VkPipelineViewportStateCreateInfo*         pViewportState
        rasterizationStateCreateInfo ?
            rasterizationStateCreateInfo :
            &rasterizationStateCreateInfoDefault, // const VkPipelineRasterizationStateCreateInfo*    pRasterizationState
        multisampleStateCreateInfo ?
            multisampleStateCreateInfo :
            &multisampleStateCreateInfoDefault, // const VkPipelineMultisampleStateCreateInfo*      pMultisampleState
        depthStencilStateCreateInfo ?
            depthStencilStateCreateInfo :
            &depthStencilStateCreateInfoDefault, // const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState
        colorBlendStateCreateInfo ?
            colorBlendStateCreateInfo :
            &colorBlendStateCreateInfoDefault, // const VkPipelineColorBlendStateCreateInfo*       pColorBlendState
        dynamicStateCreateInfo ?
            dynamicStateCreateInfo :
            dynamicStateCreateInfoDefaultPtr, // const VkPipelineDynamicStateCreateInfo*          pDynamicState
        pipelineLayout,                       // VkPipelineLayout                                 layout
        renderPass,                           // VkRenderPass                                     renderPass
        subpass,                              // uint32_t                                         subpass
        DE_NULL,                              // VkPipeline                                       basePipelineHandle
        0                                     // int32_t                                          basePipelineIndex;
    };

    return createGraphicsPipeline(vk, device, DE_NULL, &pipelineCreateInfo);
}

Move<VkPipeline> makeGraphicsPipeline(const DeviceInterface &vk, const VkDevice device,
                                      const VkPipelineLayout pipelineLayout, const VkShaderModule vertexShaderModule,
                                      const VkShaderModule tessellationControlShaderModule,
                                      const VkShaderModule tessellationEvalShaderModule,
                                      const VkShaderModule geometryShaderModule,
                                      const VkShaderModule fragmentShaderModule, const VkRenderPass renderPass,
                                      const uint32_t subpass,
                                      const VkPipelineVertexInputStateCreateInfo *vertexInputStateCreateInfo,
                                      const VkPipelineInputAssemblyStateCreateInfo *inputAssemblyStateCreateInfo,
                                      const VkPipelineTessellationStateCreateInfo *tessStateCreateInfo,
                                      const VkPipelineViewportStateCreateInfo *viewportStateCreateInfo,
                                      const VkPipelineRasterizationStateCreateInfo *rasterizationStateCreateInfo,
                                      const VkPipelineMultisampleStateCreateInfo *multisampleStateCreateInfo,
                                      const VkPipelineDepthStencilStateCreateInfo *depthStencilStateCreateInfo,
                                      const VkPipelineColorBlendStateCreateInfo *colorBlendStateCreateInfo,
                                      const VkPipelineDynamicStateCreateInfo *dynamicStateCreateInfo)
{
    VkPipelineShaderStageCreateInfo stageCreateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // VkStructureType                     sType
        DE_NULL,                                             // const void*                         pNext
        0u,                                                  // VkPipelineShaderStageCreateFlags    flags
        VK_SHADER_STAGE_VERTEX_BIT,                          // VkShaderStageFlagBits               stage
        DE_NULL,                                             // VkShaderModule                      module
        "main",                                              // const char*                         pName
        DE_NULL                                              // const VkSpecializationInfo*         pSpecializationInfo
    };

    std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageParams;

    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        stageCreateInfo.module = vertexShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    if (tessellationControlShaderModule != DE_NULL)
    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        stageCreateInfo.module = tessellationControlShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    if (tessellationEvalShaderModule != DE_NULL)
    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        stageCreateInfo.module = tessellationEvalShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    if (geometryShaderModule != DE_NULL)
    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
        stageCreateInfo.module = geometryShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    if (fragmentShaderModule != DE_NULL)
    {
        stageCreateInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        stageCreateInfo.module = fragmentShaderModule;
        pipelineShaderStageParams.push_back(stageCreateInfo);
    }

    const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, // VkStructureType                                  sType
        DE_NULL,                                         // const void*                                      pNext
        0u,                                              // VkPipelineCreateFlags                            flags
        (uint32_t)pipelineShaderStageParams.size(),      // uint32_t                                         stageCount
        &pipelineShaderStageParams[0],                   // const VkPipelineShaderStageCreateInfo*           pStages
        vertexInputStateCreateInfo,   // const VkPipelineVertexInputStateCreateInfo*      pVertexInputState
        inputAssemblyStateCreateInfo, // const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState
        tessStateCreateInfo,          // const VkPipelineTessellationStateCreateInfo*     pTessellationState
        viewportStateCreateInfo,      // const VkPipelineViewportStateCreateInfo*         pViewportState
        rasterizationStateCreateInfo, // const VkPipelineRasterizationStateCreateInfo*    pRasterizationState
        multisampleStateCreateInfo,   // const VkPipelineMultisampleStateCreateInfo*      pMultisampleState
        depthStencilStateCreateInfo,  // const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState
        colorBlendStateCreateInfo,    // const VkPipelineColorBlendStateCreateInfo*       pColorBlendState
        dynamicStateCreateInfo,       // const VkPipelineDynamicStateCreateInfo*          pDynamicState
        pipelineLayout,               // VkPipelineLayout                                 layout
        renderPass,                   // VkRenderPass                                     renderPass
        subpass,                      // uint32_t                                         subpass
        DE_NULL,                      // VkPipeline                                       basePipelineHandle
        0                             // int32_t                                          basePipelineIndex;
    };

    return createGraphicsPipeline(vk, device, DE_NULL, &pipelineCreateInfo);
}

Move<VkRenderPass> makeRenderPass(const DeviceInterface &vk, const VkDevice device, const VkFormat colorFormat,
                                  const VkFormat depthStencilFormat, const VkAttachmentLoadOp loadOperation,
                                  const VkImageLayout finalLayoutColor, const VkImageLayout finalLayoutDepthStencil,
                                  const VkImageLayout subpassLayoutColor, const VkImageLayout subpassLayoutDepthStencil,
                                  const VkAllocationCallbacks *const allocationCallbacks)
{
    const bool hasColor                           = colorFormat != VK_FORMAT_UNDEFINED;
    const bool hasDepthStencil                    = depthStencilFormat != VK_FORMAT_UNDEFINED;
    const VkImageLayout initialLayoutColor        = loadOperation == VK_ATTACHMENT_LOAD_OP_LOAD ?
                                                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
                                                        VK_IMAGE_LAYOUT_UNDEFINED;
    const VkImageLayout initialLayoutDepthStencil = loadOperation == VK_ATTACHMENT_LOAD_OP_LOAD ?
                                                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
                                                        VK_IMAGE_LAYOUT_UNDEFINED;

    const VkAttachmentDescription colorAttachmentDescription = {
        (VkAttachmentDescriptionFlags)0,  // VkAttachmentDescriptionFlags    flags
        colorFormat,                      // VkFormat                        format
        VK_SAMPLE_COUNT_1_BIT,            // VkSampleCountFlagBits           samples
        loadOperation,                    // VkAttachmentLoadOp              loadOp
        VK_ATTACHMENT_STORE_OP_STORE,     // VkAttachmentStoreOp             storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // VkAttachmentLoadOp              stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE, // VkAttachmentStoreOp             stencilStoreOp
        initialLayoutColor,               // VkImageLayout                   initialLayout
        finalLayoutColor                  // VkImageLayout                   finalLayout
    };

    const VkAttachmentDescription depthStencilAttachmentDescription = {
        (VkAttachmentDescriptionFlags)0, // VkAttachmentDescriptionFlags    flags
        depthStencilFormat,              // VkFormat                        format
        VK_SAMPLE_COUNT_1_BIT,           // VkSampleCountFlagBits           samples
        loadOperation,                   // VkAttachmentLoadOp              loadOp
        VK_ATTACHMENT_STORE_OP_STORE,    // VkAttachmentStoreOp             storeOp
        loadOperation,                   // VkAttachmentLoadOp              stencilLoadOp
        VK_ATTACHMENT_STORE_OP_STORE,    // VkAttachmentStoreOp             stencilStoreOp
        initialLayoutDepthStencil,       // VkImageLayout                   initialLayout
        finalLayoutDepthStencil          // VkImageLayout                   finalLayout
    };

    std::vector<VkAttachmentDescription> attachmentDescriptions;

    if (hasColor)
        attachmentDescriptions.push_back(colorAttachmentDescription);
    if (hasDepthStencil)
        attachmentDescriptions.push_back(depthStencilAttachmentDescription);

    const VkAttachmentReference colorAttachmentRef = {
        0u,                // uint32_t         attachment
        subpassLayoutColor // VkImageLayout    layout
    };

    const VkAttachmentReference depthStencilAttachmentRef = {
        hasColor ? 1u : 0u,       // uint32_t         attachment
        subpassLayoutDepthStencil // VkImageLayout    layout
    };

    const VkSubpassDescription subpassDescription = {
        (VkSubpassDescriptionFlags)0,             // VkSubpassDescriptionFlags       flags
        VK_PIPELINE_BIND_POINT_GRAPHICS,          // VkPipelineBindPoint             pipelineBindPoint
        0u,                                       // uint32_t                        inputAttachmentCount
        DE_NULL,                                  // const VkAttachmentReference*    pInputAttachments
        hasColor ? 1u : 0u,                       // uint32_t                        colorAttachmentCount
        hasColor ? &colorAttachmentRef : DE_NULL, // const VkAttachmentReference*    pColorAttachments
        DE_NULL,                                  // const VkAttachmentReference*    pResolveAttachments
        hasDepthStencil ? &depthStencilAttachmentRef :
                          DE_NULL, // const VkAttachmentReference*    pDepthStencilAttachment
        0u,                        // uint32_t                        preserveAttachmentCount
        DE_NULL                    // const uint32_t*                 pPreserveAttachments
    };

    const VkRenderPassCreateInfo renderPassInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, // VkStructureType                   sType
        DE_NULL,                                   // const void*                       pNext
        (VkRenderPassCreateFlags)0,                // VkRenderPassCreateFlags           flags
        (uint32_t)attachmentDescriptions.size(),   // uint32_t                          attachmentCount
        attachmentDescriptions.size() > 0 ? &attachmentDescriptions[0] :
                                            DE_NULL, // const VkAttachmentDescription*    pAttachments
        1u,                                          // uint32_t                          subpassCount
        &subpassDescription,                         // const VkSubpassDescription*       pSubpasses
        0u,                                          // uint32_t                          dependencyCount
        DE_NULL                                      // const VkSubpassDependency*        pDependencies
    };

    return createRenderPass(vk, device, &renderPassInfo, allocationCallbacks);
}

Move<VkImageView> makeImageView(const DeviceInterface &vk, const VkDevice vkDevice, const VkImage image,
                                const VkImageViewType imageViewType, const VkFormat format,
                                const VkImageSubresourceRange subresourceRange,
                                const VkImageViewUsageCreateInfo *imageUsageCreateInfo)
{
    const VkImageViewCreateInfo imageViewParams = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // VkStructureType sType;
        imageUsageCreateInfo,                     // const void* pNext;
        0u,                                       // VkImageViewCreateFlags flags;
        image,                                    // VkImage image;
        imageViewType,                            // VkImageViewType viewType;
        format,                                   // VkFormat format;
        makeComponentMappingRGBA(),               // VkComponentMapping components;
        subresourceRange,                         // VkImageSubresourceRange subresourceRange;
    };
    return createImageView(vk, vkDevice, &imageViewParams);
}

Move<VkBufferView> makeBufferView(const DeviceInterface &vk, const VkDevice vkDevice, const VkBuffer buffer,
                                  const VkFormat format, const VkDeviceSize offset, const VkDeviceSize size)
{
    const VkBufferViewCreateInfo bufferViewParams = {
        VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO, // VkStructureType sType;
        DE_NULL,                                   // const void* pNext;
        0u,                                        // VkBufferViewCreateFlags flags;
        buffer,                                    // VkBuffer buffer;
        format,                                    // VkFormat format;
        offset,                                    // VkDeviceSize offset;
        size,                                      // VkDeviceSize range;
    };
    return createBufferView(vk, vkDevice, &bufferViewParams);
}

Move<VkDescriptorSet> makeDescriptorSet(const DeviceInterface &vk, const VkDevice device,
                                        const VkDescriptorPool descriptorPool, const VkDescriptorSetLayout setLayout,
                                        const void *pNext)
{
    const VkDescriptorSetAllocateInfo allocateParams = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // VkStructureType sType;
        pNext,                                          // const void* pNext;
        descriptorPool,                                 // VkDescriptorPool descriptorPool;
        1u,                                             // uint32_t setLayoutCount;
        &setLayout,                                     // const VkDescriptorSetLayout* pSetLayouts;
    };
    return allocateDescriptorSet(vk, device, &allocateParams);
}

VkBufferCreateInfo makeBufferCreateInfo(const VkDeviceSize size, const VkBufferUsageFlags usage)
{
    const VkBufferCreateInfo bufferCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // VkStructureType sType;
        DE_NULL,                              // const void* pNext;
        (VkBufferCreateFlags)0,               // VkBufferCreateFlags flags;
        size,                                 // VkDeviceSize size;
        usage,                                // VkBufferUsageFlags usage;
        VK_SHARING_MODE_EXCLUSIVE,            // VkSharingMode sharingMode;
        0u,                                   // uint32_t queueFamilyIndexCount;
        DE_NULL,                              // const uint32_t* pQueueFamilyIndices;
    };
    return bufferCreateInfo;
}

VkBufferCreateInfo makeBufferCreateInfo(const VkDeviceSize size, const VkBufferUsageFlags usage,
                                        const std::vector<uint32_t> &queueFamilyIndices)
{
    const uint32_t queueFamilyIndexCount      = static_cast<uint32_t>(queueFamilyIndices.size());
    const uint32_t *pQueueFamilyIndices       = de::dataOrNull(queueFamilyIndices);
    const VkBufferCreateInfo bufferCreateInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // VkStructureType sType;
        DE_NULL,                              // const void* pNext;
        (VkBufferCreateFlags)0,               // VkBufferCreateFlags flags;
        size,                                 // VkDeviceSize size;
        usage,                                // VkBufferUsageFlags usage;
        VK_SHARING_MODE_EXCLUSIVE,            // VkSharingMode sharingMode;
        queueFamilyIndexCount,                // uint32_t queueFamilyIndexCount;
        pQueueFamilyIndices,                  // const uint32_t* pQueueFamilyIndices;
    };

    return bufferCreateInfo;
}

Move<VkPipelineLayout> makePipelineLayout(const DeviceInterface &vk, const VkDevice device,
                                          const VkDescriptorSetLayout descriptorSetLayout)
{
    return makePipelineLayout(vk, device, (descriptorSetLayout == DE_NULL) ? 0u : 1u, &descriptorSetLayout);
}

Move<VkPipelineLayout> makePipelineLayout(const DeviceInterface &vk, const VkDevice device,
                                          const std::vector<vk::Move<VkDescriptorSetLayout>> &descriptorSetLayouts)
{
    // Create a list of descriptor sets without move pointers.
    std::vector<vk::VkDescriptorSetLayout> descriptorSetLayoutsUnWrapped;
    for (const auto &descriptorSetLayout : descriptorSetLayouts)
    {
        descriptorSetLayoutsUnWrapped.push_back(descriptorSetLayout.get());
    }
    return vk::makePipelineLayout(vk, device, static_cast<uint32_t>(descriptorSetLayoutsUnWrapped.size()),
                                  descriptorSetLayoutsUnWrapped.data());
}

Move<VkPipelineLayout> makePipelineLayout(const DeviceInterface &vk, const VkDevice device,
                                          const uint32_t setLayoutCount,
                                          const VkDescriptorSetLayout *descriptorSetLayout)
{
    const VkPipelineLayoutCreateInfo pipelineLayoutParams = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, // VkStructureType sType;
        DE_NULL,                                       // const void* pNext;
        0u,                                            // VkPipelineLayoutCreateFlags flags;
        setLayoutCount,                                // uint32_t setLayoutCount;
        descriptorSetLayout,                           // const VkDescriptorSetLayout* pSetLayouts;
        0u,                                            // uint32_t pushConstantRangeCount;
        DE_NULL,                                       // const VkPushConstantRange* pPushConstantRanges;
    };

    return createPipelineLayout(vk, device, &pipelineLayoutParams);
}

Move<VkPipelineLayout> makePipelineLayout(const DeviceInterface &vk, const VkDevice device,
                                          const uint32_t setLayoutCount,
                                          const VkDescriptorSetLayout *descriptorSetLayout,
                                          const uint32_t pushConstantRangeCount,
                                          const VkPushConstantRange *pPushConstantRanges)
{
    const VkPipelineLayoutCreateInfo pipelineLayoutParams = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, // VkStructureType sType;
        DE_NULL,                                       // const void* pNext;
        0u,                                            // VkPipelineLayoutCreateFlags flags;
        setLayoutCount,                                // uint32_t setLayoutCount;
        descriptorSetLayout,                           // const VkDescriptorSetLayout* pSetLayouts;
        pushConstantRangeCount,                        // uint32_t pushConstantRangeCount;
        pPushConstantRanges,                           // const VkPushConstantRange* pPushConstantRanges;
    };

    return createPipelineLayout(vk, device, &pipelineLayoutParams);
}

Move<VkFramebuffer> makeFramebuffer(const DeviceInterface &vk, const VkDevice device, const VkRenderPass renderPass,
                                    const VkImageView colorAttachment, const uint32_t width, const uint32_t height,
                                    const uint32_t layers)
{
    return makeFramebuffer(vk, device, renderPass, 1u, &colorAttachment, width, height, layers);
}

Move<VkFramebuffer> makeFramebuffer(const DeviceInterface &vk, const VkDevice device, const VkRenderPass renderPass,
                                    const uint32_t attachmentCount, const VkImageView *attachmentsArray,
                                    const uint32_t width, const uint32_t height, const uint32_t layers)
{
    const VkFramebufferCreateInfo framebufferInfo = {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // VkStructureType sType;
        DE_NULL,                                   // const void* pNext;
        (VkFramebufferCreateFlags)0,               // VkFramebufferCreateFlags flags;
        renderPass,                                // VkRenderPass renderPass;
        attachmentCount,                           // uint32_t attachmentCount;
        attachmentsArray,                          // const VkImageView* pAttachments;
        width,                                     // uint32_t width;
        height,                                    // uint32_t height;
        layers,                                    // uint32_t layers;
    };

    return createFramebuffer(vk, device, &framebufferInfo);
}

Move<VkCommandPool> makeCommandPool(const DeviceInterface &vk, const VkDevice device, const uint32_t queueFamilyIndex)
{
    const VkCommandPoolCreateInfo commandPoolParams = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,      // VkStructureType sType;
        DE_NULL,                                         // const void* pNext;
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // VkCommandPoolCreateFlags flags;
        queueFamilyIndex,                                // uint32_t queueFamilyIndex;
    };

    return createCommandPool(vk, device, &commandPoolParams);
}

VkBufferImageCopy makeBufferImageCopy(const VkExtent3D extent, const VkImageSubresourceLayers subresourceLayers)
{
    const VkBufferImageCopy copyParams = {
        0ull,                  // VkDeviceSize bufferOffset;
        0u,                    // uint32_t bufferRowLength;
        0u,                    // uint32_t bufferImageHeight;
        subresourceLayers,     // VkImageSubresourceLayers imageSubresource;
        makeOffset3D(0, 0, 0), // VkOffset3D imageOffset;
        extent,                // VkExtent3D imageExtent;
    };
    return copyParams;
}

} // namespace vk
