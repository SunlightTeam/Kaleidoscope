#pragma once


class GLFWwindow;
class VulkanGraphicDriver;


class WindowsApplication
{
public:
	WindowsApplication();
	virtual ~WindowsApplication();

	void Run();

	void SetWindowResized(bool resized) { m_WindowResized = resized; }

private:
	virtual int Initial();
	virtual int StartUp();
	virtual int MainLoop();
	virtual void DrawFrame();
	virtual void ResizeWindow();
	virtual int ShutDown();
	virtual void CleanUp();

	/**
	 * Windows system
	 */

	GLFWwindow*               m_MainWindow;
	bool                      m_WindowResized;
	 
	/**
	 *  Vulkan
	 */
	VulkanGraphicDriver*      m_GraphicDriver;
};

