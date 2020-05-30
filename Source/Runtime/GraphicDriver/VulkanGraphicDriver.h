#pragma once


class GLFWwindow;


__BEGIN_NAMESPACE


typedef struct GraphicInitialInfo {
	GLFWwindow* m_Window;
	int         m_Width;
	int         m_Height;
} GraphicInitialInfo;

typedef struct GraphicResizeInfo {
	int         m_OldWidth;
	int         m_OldHeight;
	int         m_NewWidth;
	int         m_NewHeight;
} GraphicResizeInfo;


class VulkanGraphicDriver
{
public:
	VulkanGraphicDriver();
	virtual ~VulkanGraphicDriver();

	virtual bool Initial();
	virtual bool StartUp(const GraphicInitialInfo& initialInfo);
	virtual bool DrawFrame();
	virtual bool ResizeWindow(const GraphicResizeInfo& resizeInfo);
	virtual bool ShutDown();
	virtual void CleanUp();

private:
	VkInstance                        m_VulkanInstance;
	VkSurfaceKHR                      m_VulkanWindowSurface;
	VkPhysicalDevice                  m_VulkanPhysicalDevice;
	uint32_t                          m_VulkanGraphicQueueFamilyID;
	uint32_t                          m_VulkanPresentQueueFamilyID;
	VkSurfaceCapabilitiesKHR          m_SurfaceCapabilities;
	std::vector<VkSurfaceFormatKHR>   m_SurfaceFormats;
	std::vector<VkPresentModeKHR>     m_PresentModes;
	VkDevice                          m_VulkanLogicDevice;
	VkQueue                           m_VulkanGraphicQueue;
	VkQueue                           m_VulkanPresentQueue;
	VkSurfaceFormatKHR                m_VulkanSurfaceFormat;
	VkPresentModeKHR                  m_VulkanSwapPresentMode;
	VkExtent2D                        m_VulkanSwapExtent;
	VkSwapchainKHR                    m_VulkanSwapChain;
	std::vector<VkImage>              m_VulkanSwapChainImages;
	std::vector<VkImageView>          m_VulkanSwapChainImageViews;
	VkRenderPass                      m_VulkanRenderPass;
	VkPipelineLayout                  m_VulkanPipelineLayout;
	VkPipeline                        m_VulkanGraphicsPipeline;
	std::vector<VkFramebuffer>        m_VulkanSwapChainFramebuffers;
	VkCommandPool                     m_VulkanCommandPool;
	std::vector<VkCommandBuffer>      m_VulkanCommandBuffers;

	std::vector<VkSemaphore>          m_ImageAvailableSemaphores;
	std::vector<VkSemaphore>          m_RenderFinishedSemaphores;
	std::vector<VkFence>              m_InFlightFences;
	std::vector<VkFence>              m_InFlightImageFences;
	size_t                            m_CurrentFrame;
	bool                              m_SkipFrame;

	bool CreateSwapChain(uint32_t width, uint32_t height);
	bool CreateShaderAndPipeline();
	bool CreateFrameBuffers();
	bool CreateCommandBuffers();	
	bool DestroyCommandBuffers();
	bool DestroyFrameBuffers();
	bool DestroyShaderAndPipeline();		
	bool DestroySwapChain();
};


__END_NAMESPACE
