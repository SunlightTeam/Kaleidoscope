#include <cstdint> // Necessary for UINT32_MAX
#include "VulkanGraphicDriver.h"


__BEGIN_NAMESPACE

static const size_t MAX_FRAMES_IN_FLIGHT = 2;
static const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation"};
static const bool enableValidationLayers = true; 

#define VULKAN_DRIVER_CHECK_FUN(fun) {bool ok = fun; \
if (!ok) { \
    return false; \
}}

static bool CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

static int RateDeviceSuitabilityScore(VkPhysicalDevice device) 
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    return score;
}

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t w, uint32_t h)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }  
    std::cout << "No valid swapchain extend, using default width & height.\n";

    VkExtent2D actualExtent = { w, h };
    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
    return actualExtent;
}

static int ReadFile(const std::string& filename, std::vector<char>& output)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cout << "failed to open file!\n";
        return -1;
    }

    size_t fileSize = (size_t)file.tellg();
    output.resize(fileSize);
    file.seekg(0);
    file.read(output.data(), fileSize);
    file.close();
    return 0;
}

std::string CurExePath()
{
    char buffer[MAX_PATH];
    ::GetModuleFileName(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << "Vulkan failed to create shader module.\n";
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}


VulkanGraphicDriver::VulkanGraphicDriver():
	m_VulkanInstance(VK_NULL_HANDLE),
	m_VulkanWindowSurface(VK_NULL_HANDLE),
	m_VulkanPhysicalDevice(VK_NULL_HANDLE),
	m_VulkanGraphicQueueFamilyID(UINT32_MAX),
	m_VulkanPresentQueueFamilyID(UINT32_MAX),
	m_VulkanLogicDevice(VK_NULL_HANDLE),
	m_VulkanGraphicQueue(VK_NULL_HANDLE),
	m_VulkanPresentQueue(VK_NULL_HANDLE),
	m_VulkanSwapPresentMode(VK_PRESENT_MODE_FIFO_KHR),
	m_VulkanSwapChain(VK_NULL_HANDLE),
	m_VulkanRenderPass(VK_NULL_HANDLE),
	m_VulkanPipelineLayout(VK_NULL_HANDLE),
	m_VulkanGraphicsPipeline(VK_NULL_HANDLE),
	m_VulkanCommandPool(VK_NULL_HANDLE),
	m_CurrentFrame(0),
    m_SkipFrame(false)
{
    m_VulkanSurfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
    m_VulkanSurfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    m_VulkanSwapExtent.width = 0;
    m_VulkanSwapExtent.height = 0;
}

VulkanGraphicDriver::~VulkanGraphicDriver()
{

}

bool VulkanGraphicDriver::Initial()
{
    return true;
}

/****************************************************************************
 * Create swapchain
 ****************************************************************************/
bool VulkanGraphicDriver::CreateSwapChain(uint32_t width, uint32_t height)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_VulkanPhysicalDevice, m_VulkanWindowSurface, &m_SurfaceCapabilities);
    m_VulkanSwapExtent = ChooseSwapExtent(m_SurfaceCapabilities, width, height);

    uint32_t imageCount = m_SurfaceCapabilities.minImageCount + 1;
    if (m_SurfaceCapabilities.maxImageCount > 0 && imageCount > m_SurfaceCapabilities.maxImageCount) {
        imageCount = m_SurfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_VulkanWindowSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = m_VulkanSurfaceFormat.format;
    createInfo.imageColorSpace = m_VulkanSurfaceFormat.colorSpace;
    createInfo.imageExtent = m_VulkanSwapExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (m_VulkanGraphicQueueFamilyID != m_VulkanPresentQueueFamilyID) {
        uint32_t queueFamilyIndices[] = { m_VulkanGraphicQueueFamilyID, m_VulkanPresentQueueFamilyID };
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    createInfo.preTransform = m_SurfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = m_VulkanSwapPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_VulkanLogicDevice, &createInfo, nullptr, &m_VulkanSwapChain) != VK_SUCCESS) {
        std::cout << "Failed to create Vulkan swap chain.\n";
        SetErrorCode(ErrorCode::Vulkan_Invalid_SwapChain);
        return false;
    }

    vkGetSwapchainImagesKHR(m_VulkanLogicDevice, m_VulkanSwapChain, &imageCount, nullptr);
    m_VulkanSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_VulkanLogicDevice, m_VulkanSwapChain, &imageCount, m_VulkanSwapChainImages.data());
    m_VulkanSwapChainImageViews.resize(m_VulkanSwapChainImages.size());

    for (size_t i = 0; i < m_VulkanSwapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_VulkanSwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_VulkanSurfaceFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_VulkanLogicDevice, &createInfo, nullptr, &m_VulkanSwapChainImageViews[i]) != VK_SUCCESS) {
            std::cout << "Failed to create vulkan swapchain image views.\n";
            SetErrorCode(ErrorCode::Vulkan_InvalidSwapChainImageView);
            return false;
        }
    }
    return true;
}

/****************************************************************************
 * Create shader and pipeline
 ****************************************************************************/
bool VulkanGraphicDriver::CreateShaderAndPipeline()
{
    std::vector<char> vertShaderCode;
    std::string curExePath = CurExePath();
    ReadFile(curExePath + "/../Data/Engine/sample.vs.spv", vertShaderCode);
    std::vector<char> fragShaderCode;
    ReadFile(curExePath + "/../Data/Engine/sample.fs.spv", fragShaderCode);

    VkShaderModule vertShaderModule = CreateShaderModule(m_VulkanLogicDevice, vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(m_VulkanLogicDevice, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_VulkanSwapExtent.width;
    viewport.height = (float)m_VulkanSwapExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_VulkanSwapExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    //VkPipelineDepthStencilStateCreateInfo

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    /*
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    */

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(m_VulkanLogicDevice, &pipelineLayoutInfo, nullptr, &m_VulkanPipelineLayout) != VK_SUCCESS) {
        std::cout << "Vulkan failed to create pipeline layout.\n";
        SetErrorCode(ErrorCode::UnKnow);
        return false;
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_VulkanSurfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_VulkanLogicDevice, &renderPassInfo, nullptr, &m_VulkanRenderPass) != VK_SUCCESS) {
        std::cout << "Vulkan failed to create render pass.\n";
        SetErrorCode(ErrorCode::UnKnow);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = m_VulkanPipelineLayout;
    pipelineInfo.renderPass = m_VulkanRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(m_VulkanLogicDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VulkanGraphicsPipeline) != VK_SUCCESS) {
        std::cout << "Vulkan failed to create graphics pipeline.\n";
        SetErrorCode(ErrorCode::UnKnow);
        return false;
    }

    vkDestroyShaderModule(m_VulkanLogicDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_VulkanLogicDevice, vertShaderModule, nullptr);
    return true;
}

/****************************************************************************
 * Create framebuffers
 ****************************************************************************/
bool VulkanGraphicDriver::CreateFrameBuffers()
{
    m_VulkanSwapChainFramebuffers.resize(m_VulkanSwapChainImageViews.size());
    for (size_t i = 0; i < m_VulkanSwapChainImageViews.size(); i++) {
        VkImageView attachments[] = { m_VulkanSwapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_VulkanRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_VulkanSwapExtent.width;
        framebufferInfo.height = m_VulkanSwapExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_VulkanLogicDevice, &framebufferInfo, nullptr, &m_VulkanSwapChainFramebuffers[i]) != VK_SUCCESS) {
            std::cout << "Vulkan failed to create framebuffer.\n";
            SetErrorCode(ErrorCode::UnKnow);
            return false;
        }
    }
    return true;
}

/****************************************************************************
* Create command buffers
****************************************************************************/
bool VulkanGraphicDriver::CreateCommandBuffers()
{
    m_VulkanCommandBuffers.resize(m_VulkanSwapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_VulkanCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_VulkanCommandBuffers.size();

    if (vkAllocateCommandBuffers(m_VulkanLogicDevice, &allocInfo, m_VulkanCommandBuffers.data()) != VK_SUCCESS) {
        std::cout << "Vulkan failed to allocate command buffers.\n";
        SetErrorCode(ErrorCode::UnKnow);
        return false;
    }

    for (size_t i = 0; i < m_VulkanCommandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(m_VulkanCommandBuffers[i], &beginInfo) != VK_SUCCESS) {
            std::cout << "Vulkan failed to begin recording command buffer.\n";
            SetErrorCode(ErrorCode::UnKnow);
            return false;
        }
    }

    for (size_t i = 0; i < m_VulkanCommandBuffers.size(); i++)
    {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_VulkanRenderPass;
        renderPassInfo.framebuffer = m_VulkanSwapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_VulkanSwapExtent;

        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_VulkanCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(m_VulkanCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanGraphicsPipeline);
        vkCmdDraw(m_VulkanCommandBuffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(m_VulkanCommandBuffers[i]);

        if (vkEndCommandBuffer(m_VulkanCommandBuffers[i]) != VK_SUCCESS) {
            std::cout << "Vulkan failed to record command buffer.\n";
            SetErrorCode(ErrorCode::UnKnow);
            return false;
        }
    }
    return true;
}

bool VulkanGraphicDriver::StartUp(const GraphicInitialInfo& initialInfo)
{
    if (nullptr == initialInfo.m_Window) {
        std::cout << "Input window is null.";
        SetErrorCode(ErrorCode::UnKnow);
        return false;
    }

    // enumerate all instance extension
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    std::cout << "Vulkan instance created, available vulkan extensions:\n";
    for (const auto& extension : extensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }

    /*******************************************************************************************
    * Create instance
    *******************************************************************************************/
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Player";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pEngineName = "Kaleidoscope";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        bool useValidationLayers = enableValidationLayers;
        if (enableValidationLayers && !CheckValidationLayerSupport()) {
            std::cout << "Vulkan ValidationLayer doesn't supported.\n";
            useValidationLayers = false;
        }

        if (useValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance);
        if (VK_SUCCESS != result)
        {
            std::cout << "Vulkan instance creation failed.\n";
            SetErrorCode(ErrorCode::Vulkan_Invalid_Instance);
            return false;
        }
    }

    if (glfwCreateWindowSurface(m_VulkanInstance, initialInfo.m_Window, nullptr, &m_VulkanWindowSurface) != VK_SUCCESS) {
        std::cout << "Vulkan window surface creation failed.\n";
        SetErrorCode(ErrorCode::Vulkan_Invalid_WindowSurface);
        return false;
    }

    /*******************************************************************************************
     * Pick physical device & queue family & logic device and queue
     *******************************************************************************************/
    {
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, nullptr);
        if (deviceCount <= 0) {
            std::cout << "Vulkan no physical device.\n";
            SetErrorCode(ErrorCode::Vulkan_No_PhysicalDevice);
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, devices.data());

        int maxDeviceScore = 1; //SKIP '0' score
        //assert(VK_NULL_HANDLE==m_VulkanPhysicalDevice);
        for (const auto& device : devices) {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
            for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }

            // swapchain extension supported
            if (!requiredExtensions.empty()) {
                continue;
            }

            int deviceScore = RateDeviceSuitabilityScore(device);
            if (deviceScore > maxDeviceScore) {
                m_VulkanPhysicalDevice = device;
                break;
            }
        }

        if (VK_NULL_HANDLE == m_VulkanPhysicalDevice) {
            std::cout << "Vulkan no suitabel physical device.\n";
            SetErrorCode(ErrorCode::Vulkan_Invalid_PhysicalDevice);
            return false;
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_VulkanPhysicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_VulkanPhysicalDevice, &queueFamilyCount, queueFamilies.data());

        for (uint32_t qfid = 0; qfid < queueFamilyCount; ++qfid) {
            if (queueFamilies[qfid].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_VulkanGraphicQueueFamilyID = qfid;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_VulkanPhysicalDevice, qfid, m_VulkanWindowSurface, &presentSupport);
            if (presentSupport) {
                m_VulkanPresentQueueFamilyID = qfid;
            }

            if ((UINT32_MAX != m_VulkanGraphicQueueFamilyID) && (UINT32_MAX != m_VulkanPresentQueueFamilyID)) {
                break;
            }
        }

        if ((UINT32_MAX == m_VulkanGraphicQueueFamilyID)) {
            std::cout << "Vulkan physical device doesn't have graphic queue family.\n";
            SetErrorCode(ErrorCode::Vulkan_No_GraphicQueueFamily);
            return false;
        }

        if ((UINT32_MAX == m_VulkanPresentQueueFamilyID))
        {
            std::cout << "Vulkan physical device doesn't have present queue family.\n";
            SetErrorCode(ErrorCode::Vulkan_No_PresentQueueFamily);
            return false;
        }

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_VulkanPhysicalDevice, m_VulkanWindowSurface, &formatCount, nullptr);
        if (formatCount != 0) {
            m_SurfaceFormats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_VulkanPhysicalDevice, m_VulkanWindowSurface, &formatCount, m_SurfaceFormats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_VulkanPhysicalDevice, m_VulkanWindowSurface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            m_PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_VulkanPhysicalDevice, m_VulkanWindowSurface, &presentModeCount, m_PresentModes.data());
        }

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = m_VulkanGraphicQueueFamilyID;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = m_VulkanPresentQueueFamilyID;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        // Don't set validateLayer
        createInfo.enabledLayerCount = 0;

        if (vkCreateDevice(m_VulkanPhysicalDevice, &createInfo, nullptr, &m_VulkanLogicDevice) != VK_SUCCESS) {
            std::cout << "Vulkan failed to create logic device.\n";
            SetErrorCode(ErrorCode::Vulkan_Invalid_LogicDevice);
            return false;
        }

        vkGetDeviceQueue(m_VulkanLogicDevice, m_VulkanGraphicQueueFamilyID, 0, &m_VulkanGraphicQueue);
        vkGetDeviceQueue(m_VulkanLogicDevice, m_VulkanPresentQueueFamilyID, 0, &m_VulkanPresentQueue);


        // swapchain detail supported
        if (m_SurfaceFormats.empty()) {
            std::cout << "Vulkan physical device doesn't has nessecery swapchain format.\n";
            SetErrorCode(ErrorCode::Vulkan_Invalid_SwapChainFormat);
            return false;
        }
        if (m_PresentModes.empty()) {
            std::cout << "Vulkan physical device doesn't has nessecery swapchain presentmode.\n";
            SetErrorCode(ErrorCode::Vulkan_Invalid_SwapChainPresentMode);
            return false;
        }

        m_VulkanSurfaceFormat = ChooseSwapSurfaceFormat(m_SurfaceFormats);
        m_VulkanSwapPresentMode = ChooseSwapPresentMode(m_PresentModes);
    }

    /****************************************************************************
     * Create command Pool
     ****************************************************************************/
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = m_VulkanGraphicQueueFamilyID;
        poolInfo.flags = 0; // Optional

        if (vkCreateCommandPool(m_VulkanLogicDevice, &poolInfo, nullptr, &m_VulkanCommandPool) != VK_SUCCESS) {
            std::cout << "Vulkan failed to create command pool.\n";
            SetErrorCode(ErrorCode::Vulkan_Invalid_CommandPool);
            return false;
        }
    }

    uint32_t w = (uint32_t)initialInfo.m_Width;
    uint32_t h = (uint32_t)initialInfo.m_Height;

    VULKAN_DRIVER_CHECK_FUN(CreateSwapChain(w,h));
    VULKAN_DRIVER_CHECK_FUN(CreateShaderAndPipeline());
    VULKAN_DRIVER_CHECK_FUN(CreateFrameBuffers());
    VULKAN_DRIVER_CHECK_FUN(CreateCommandBuffers());

    /****************************************************************************
    * Create synchronization objects
    ****************************************************************************/
    {
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightImageFences.resize(m_VulkanSwapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(m_VulkanLogicDevice, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_VulkanLogicDevice, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_VulkanLogicDevice, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
            {

                std::cout << "Vulkan failed to create semaphores.";
                SetErrorCode(ErrorCode::UnKnow);
                return false;
            }
        }
    }

    std::cout << "Vulkan startup success.\n";
    return true;
}

bool VulkanGraphicDriver::DrawFrame()
{
    if (m_SkipFrame) {
        return true;
    }

    vkWaitForFences(m_VulkanLogicDevice, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(m_VulkanLogicDevice, m_VulkanSwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        std::cout << "Vulkan skip a frame draw because of window resize happened.\n";
        SetErrorCode(ErrorCode::UnKnow);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cout << "Vulkan failed to acquire swap chain image.\n";
        SetErrorCode(ErrorCode::UnKnow);
        return false;
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (m_InFlightImageFences[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_VulkanLogicDevice, 1, &m_InFlightImageFences[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    m_InFlightImageFences[imageIndex] = m_InFlightFences[m_CurrentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_VulkanCommandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(m_VulkanLogicDevice, 1, &m_InFlightFences[m_CurrentFrame]);

    if (vkQueueSubmit(m_VulkanGraphicQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
        std::cout << "Vulkan failed to submit draw command buffer.\n";
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_VulkanSwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    vkQueuePresentKHR(m_VulkanPresentQueue, &presentInfo);
    vkQueueWaitIdle(m_VulkanPresentQueue);

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

bool VulkanGraphicDriver::ResizeWindow(const GraphicResizeInfo& resizeInfo)
{
    // When minimize window or restore window, Only skip frame rendering and don't need re-create swapchain.
    if (0 == resizeInfo.m_NewWidth || 0 == resizeInfo.m_NewHeight) {
        m_SkipFrame = true;
        return true;
    }

    if (0 == resizeInfo.m_OldWidth || 0 == resizeInfo.m_OldHeight) {
        m_SkipFrame = false;
        return true;
    }

    vkDeviceWaitIdle(m_VulkanLogicDevice);

    DestroyCommandBuffers();
    DestroyFrameBuffers();
    DestroyShaderAndPipeline();
    DestroySwapChain();

    CreateSwapChain((uint32_t)resizeInfo.m_NewWidth, (uint32_t)resizeInfo.m_NewHeight);
    CreateShaderAndPipeline();
    CreateFrameBuffers();
    CreateCommandBuffers();

    return true;
}

bool VulkanGraphicDriver::DestroyCommandBuffers()
{
    if (!m_VulkanCommandBuffers.empty()) {
        vkFreeCommandBuffers(m_VulkanLogicDevice, m_VulkanCommandPool, static_cast<uint32_t>(m_VulkanCommandBuffers.size()), m_VulkanCommandBuffers.data());
        m_VulkanCommandBuffers.clear();
    }
    return true;
}

bool VulkanGraphicDriver::DestroyFrameBuffers()
{
    if (!m_VulkanSwapChainFramebuffers.empty()) {
        for (size_t i = 0; i < m_VulkanSwapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(m_VulkanLogicDevice, m_VulkanSwapChainFramebuffers[i], nullptr);
        }
        m_VulkanSwapChainFramebuffers.clear();
    }
    return true;
}

bool VulkanGraphicDriver::DestroyShaderAndPipeline()
{
    if (VK_NULL_HANDLE != m_VulkanRenderPass) {
        vkDestroyRenderPass(m_VulkanLogicDevice, m_VulkanRenderPass, nullptr);
        m_VulkanRenderPass = VK_NULL_HANDLE;
    }

    if (VK_NULL_HANDLE != m_VulkanGraphicsPipeline) {
        vkDestroyPipeline(m_VulkanLogicDevice, m_VulkanGraphicsPipeline, nullptr);
        m_VulkanGraphicsPipeline = VK_NULL_HANDLE;
    }

    if (VK_NULL_HANDLE != m_VulkanPipelineLayout) {
        vkDestroyPipelineLayout(m_VulkanLogicDevice, m_VulkanPipelineLayout, nullptr);
        m_VulkanPipelineLayout = VK_NULL_HANDLE;
    }
    return true;
}

bool VulkanGraphicDriver::DestroySwapChain()
{
    if (!m_VulkanSwapChainImageViews.empty()) {
        for (size_t i = 0; i < m_VulkanSwapChainImageViews.size(); i++) {
            vkDestroyImageView(m_VulkanLogicDevice, m_VulkanSwapChainImageViews[i], nullptr);
        }
        m_VulkanSwapChainImageViews.clear();
    }

    if (VK_NULL_HANDLE != m_VulkanSwapChain) {
        vkDestroySwapchainKHR(m_VulkanLogicDevice, m_VulkanSwapChain, nullptr);
        m_VulkanSwapChain = VK_NULL_HANDLE;
    }
    return true;
}

bool VulkanGraphicDriver::ShutDown()
{
    vkDeviceWaitIdle(m_VulkanLogicDevice);

    DestroyCommandBuffers();
    DestroyFrameBuffers();
    DestroyShaderAndPipeline();
    DestroySwapChain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_VulkanLogicDevice, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_VulkanLogicDevice, m_RenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_VulkanLogicDevice, m_InFlightFences[i], nullptr);
    }
    m_ImageAvailableSemaphores.clear();
    m_RenderFinishedSemaphores.clear();
    m_InFlightFences.clear();

    vkDestroyCommandPool(m_VulkanLogicDevice, m_VulkanCommandPool, nullptr);
    m_VulkanCommandPool = VK_NULL_HANDLE;

    vkDestroyDevice(m_VulkanLogicDevice, nullptr);
    m_VulkanLogicDevice = VK_NULL_HANDLE;

    vkDestroySurfaceKHR(m_VulkanInstance, m_VulkanWindowSurface, nullptr);
    m_VulkanWindowSurface = VK_NULL_HANDLE;

    vkDestroyInstance(m_VulkanInstance, nullptr);
    m_VulkanInstance = VK_NULL_HANDLE;

    return true;
}

void VulkanGraphicDriver::CleanUp()
{

}


__END_NAMESPACE