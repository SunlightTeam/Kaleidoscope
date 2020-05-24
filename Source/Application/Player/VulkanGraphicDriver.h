#pragma once


#include "GraphicDriverError.h"


class GLFWwindow;

class VulkanGraphicDriver
{
public:
	VulkanGraphicDriver();
	virtual ~VulkanGraphicDriver();

	virtual GraphicDriverErrorCode Initial();
	virtual GraphicDriverErrorCode StartUp(GLFWwindow* window);
	virtual GraphicDriverErrorCode DrawFrame();
	virtual GraphicDriverErrorCode ResizeWindow();
	virtual GraphicDriverErrorCode ShutDown();
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

	GraphicDriverErrorCode CreateSwapChain();
	GraphicDriverErrorCode CreateShaderAndPipeline();
	GraphicDriverErrorCode CreateFrameBuffers();
	GraphicDriverErrorCode CreateCommandBuffers();	
	GraphicDriverErrorCode DestroyCommandBuffers();
	GraphicDriverErrorCode DestroyFrameBuffers();
	GraphicDriverErrorCode DestroyShaderAndPipeline();		
	GraphicDriverErrorCode DestroySwapChain();
};




