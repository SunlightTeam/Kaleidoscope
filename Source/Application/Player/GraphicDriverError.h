#pragma once


enum class GraphicDriverErrorCode
{
	OK                                                       = 0,
	UnKnow                                                   = -1,

	// Vulkan special error
	Vulkan_Invalid_Instance                                  = -19999,
	Vulkan_Invalid_WindowSurface,
	Vulkan_No_PhysicalDevice,
	Vulkan_Invalid_PhysicalDevice,
	Vulkan_No_GraphicQueueFamily,
	Vulkan_No_PresentQueueFamily,
	Vulkan_Invalid_SwapChainFormat,
	Vulkan_Invalid_SwapChainPresentMode,
	Vulkan_Invalid_LogicDevice,
	Vulkan_Invalid_SwapChain,
	Vulkan_InvalidSwapChainImageView,
	Vulkan_Invalid_CommandPool,
};