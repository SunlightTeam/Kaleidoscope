class GLFWwindow;


class WindowsApplication
{
public:
	WindowsApplication():
		m_MainWindow(nullptr), 
		m_VulkanInstance(VK_NULL_HANDLE), 
		m_VulkanPhysicalDevice(VK_NULL_HANDLE),
		m_VulkanQueueFamilyID(kInvalidQueueFamilyID),
		m_VulkanLogicDevice(VK_NULL_HANDLE),
		m_VulkanGraphicQueue(VK_NULL_HANDLE)
	{}
	virtual ~WindowsApplication() {}

	void Run();

private:
	virtual int Initial();
	virtual int StartUp();
	virtual int MainLoop();
	virtual int ShutDown();
	virtual void CleanUp();

	/**
	 * Windows system
	 */

	GLFWwindow* m_MainWindow;

	/**
	 *  Vulkan
	 */
	const uint32_t kInvalidQueueFamilyID = 0xFFFFFFFF;
	VkInstance       m_VulkanInstance;
	VkPhysicalDevice m_VulkanPhysicalDevice;
	uint32_t         m_VulkanQueueFamilyID;
	VkDevice         m_VulkanLogicDevice;
	VkQueue          m_VulkanGraphicQueue;


	int StartUp_Vulkan();
	int ShutDown_Vulkan();
};

