#include <cstdint> // Necessary for UINT32_MAX
#include "VulkanGraphicDriver.h"
#include "WindowsApplication.h"


__BEGIN_NAMESPACE


static const uint32_t WIDTH = 1024;
static const uint32_t HEIGHT = 768;


WindowsApplication::WindowsApplication() :
    m_MainWindow(nullptr),
    m_GraphicDriver(nullptr),
    m_WindowResized(false),
    m_CurrentWidth(-1),
    m_CurrentHeight(-1)
{
}

WindowsApplication::~WindowsApplication()
{
}

void WindowsApplication::Run()
{
    if (!Initial()) {
        return;
    }

    if (!StartUp()) {
        return;
    }

    MainLoop();

    ShutDown();
    CleanUp();
}

bool WindowsApplication::Initial()
{
    m_GraphicDriver = new VulkanGraphicDriver();
    if (!m_GraphicDriver->Initial()) {
        return false;
    }
    return true;
}


static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<WindowsApplication*>(glfwGetWindowUserPointer(window));
    app->SetWindowResized(true);
}

bool WindowsApplication::StartUp()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_MainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Main window", nullptr, nullptr);
    glfwSetWindowUserPointer(m_MainWindow, this);
    glfwSetFramebufferSizeCallback(m_MainWindow, framebufferResizeCallback);
    m_CurrentWidth = WIDTH;
    m_CurrentHeight = HEIGHT;

    GraphicInitialInfo graphicInitialInfo;
    graphicInitialInfo.m_Window = m_MainWindow;
    if (!m_GraphicDriver->StartUp(graphicInitialInfo))
    {
        return false;
    }

    return true;
}


bool WindowsApplication::MainLoop()
{
    while (!glfwWindowShouldClose(m_MainWindow)) {
        glfwPollEvents();

        if (m_WindowResized) {
            ResizeWindow();
            m_WindowResized = false;
        } else {
            DrawFrame();
        }
    }    
    return true;
}

void WindowsApplication::DrawFrame()
{
    m_GraphicDriver->DrawFrame();
}

void WindowsApplication::ResizeWindow()
{
    GraphicResizeInfo graphicResizeInfo;
    graphicResizeInfo.m_OldWidth = m_CurrentWidth;
    graphicResizeInfo.m_OldHeight = m_CurrentHeight;

    int width, height;
    glfwGetFramebufferSize(m_MainWindow, &width, &height);
    graphicResizeInfo.m_NewWidth = (uint32_t)width;
    graphicResizeInfo.m_NewHeight = (uint32_t)height;
    m_CurrentWidth = width;
    m_CurrentHeight = height;

    m_GraphicDriver->ResizeWindow(graphicResizeInfo);
}

bool WindowsApplication::ShutDown()
{
    m_GraphicDriver->ShutDown();

    glfwDestroyWindow(m_MainWindow);
    m_MainWindow = nullptr;

    glfwTerminate();
    return true;
}

void WindowsApplication::CleanUp()
{
    m_GraphicDriver->CleanUp();
    m_GraphicDriver = nullptr;
}


__END_NAMESPACE
