#include "WindowsApplication.h"


static const uint32_t WIDTH = 1024;
static const uint32_t HEIGHT = 768;

static const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

static const bool enableValidationLayers = true;


void WindowsApplication::Run()
{
    if (Initial() < 0) {
        return;
    }

    if (StartUp() < 0) {
        return;
    }

    MainLoop();

    ShutDown();
    CleanUp();
}

int WindowsApplication::Initial()
{
    return 0;
}

int WindowsApplication::StartUp()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_MainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);

    int vulkanStartup = StartUp_Vulkan();
    if (vulkanStartup < 0)
    {
        return vulkanStartup;
    }

    return 0;
}

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

static int RateDeviceSuitabilityScore(VkPhysicalDevice device) {
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

int WindowsApplication::StartUp_Vulkan()
{
    /****************************************************************************
     * Create Vulkan Instace
     ****************************************************************************/
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
            return -1;
        }


        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        std::cout << "Vulkan instance created, available vulkan extensions:\n";
        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }

    /****************************************************************************
     * Pick physical device & queue family
     ****************************************************************************/
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, nullptr);
        if (deviceCount <= 0) {
            std::cout << "Vulkan no physical device.\n";
            return -2;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, devices.data());

        int maxDeviceScore = 1; //SKIP '0' score
        //assert(VK_NULL_HANDLE==m_VulkanPhysicalDevice);
        for (const auto& device : devices) {
            int deviceScore = RateDeviceSuitabilityScore(device);
            if (deviceScore > maxDeviceScore) {
                m_VulkanPhysicalDevice = device;
                break;
            }
        }

        if (VK_NULL_HANDLE == m_VulkanPhysicalDevice) {
            std::cout << "Vulkan no suitabel physical device.\n";
            return -3;
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_VulkanPhysicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_VulkanPhysicalDevice, &queueFamilyCount, queueFamilies.data());

        //assert(kInvalidQueueFamilyID==m_VulkanQueueFamilyID);    
        for (uint32_t qfid = 0; qfid < queueFamilyCount; ++qfid) {
            if (queueFamilies[qfid].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_VulkanQueueFamilyID = qfid;
                break;
            }
        }

        if (kInvalidQueueFamilyID == m_VulkanQueueFamilyID) {
            std::cout << "Vulkan physical device doesn't have graphic queue family.\n";
            return -4;
        }
    }

    /****************************************************************************
     * Create logic device and queue
     ****************************************************************************/
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = m_VulkanQueueFamilyID;
        queueCreateInfo.queueCount = 1;

        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;
        // Don't set validateLayer
        createInfo.enabledLayerCount = 0;

        if (vkCreateDevice(m_VulkanPhysicalDevice, &createInfo, nullptr, &m_VulkanLogicDevice) != VK_SUCCESS) {
            std::cout << "Vulkan failed to create logic device.\n";
            return -5;
        }

        vkGetDeviceQueue(m_VulkanLogicDevice, m_VulkanQueueFamilyID, 0, &m_VulkanGraphicQueue);
    }

    std::cout << "Vulkan startup success.\n";
    return 0;
}

int WindowsApplication::MainLoop()
{
    while (!glfwWindowShouldClose(m_MainWindow)) {
        glfwPollEvents();
    }

    return 0;
}

int WindowsApplication::ShutDown()
{
    ShutDown_Vulkan();

    glfwDestroyWindow(m_MainWindow);
    m_MainWindow = nullptr;

    glfwTerminate();
    return 0;
}

int WindowsApplication::ShutDown_Vulkan()
{
    vkDestroyDevice(m_VulkanLogicDevice, nullptr);
    vkDestroyInstance(m_VulkanInstance, nullptr);
    m_VulkanInstance = VK_NULL_HANDLE;
    return 0;
}

void WindowsApplication::CleanUp()
{

}