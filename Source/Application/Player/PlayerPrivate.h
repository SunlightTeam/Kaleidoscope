#pragma once


#include <windows.h>


#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>


#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <vec4.hpp>
#include <mat4x4.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <fstream>
#include <filesystem>
