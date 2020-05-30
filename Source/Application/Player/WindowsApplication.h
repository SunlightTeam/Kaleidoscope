#pragma once


class GLFWwindow;


__BEGIN_NAMESPACE


class VulkanGraphicDriver;


class WindowsApplication
{
public:
	WindowsApplication();
	virtual ~WindowsApplication();

	void Run();

	void SetWindowResized(bool resized) { m_WindowResized = resized; }

private:
	virtual bool Initial();
	virtual bool StartUp();
	virtual bool MainLoop();
	virtual void DrawFrame();
	virtual void ResizeWindow();
	virtual bool ShutDown();
	virtual void CleanUp();

	/**
	 * Windows system
	 */

	GLFWwindow*               m_MainWindow;
	bool                      m_WindowResized;
	int                       m_CurrentWidth;
	int                       m_CurrentHeight;
	 
	/**
	 *  Vulkan
	 */
	VulkanGraphicDriver*      m_GraphicDriver;
};


__END_NAMESPACE

