#include <cstdint> // Necessary for UINT32_MAX
#include "VulkanGraphicDriver.h"
#include "WindowsApplication.h"


static const uint32_t WIDTH = 1024;
static const uint32_t HEIGHT = 768;


WindowsApplication::WindowsApplication() :
    m_MainWindow(nullptr),
    m_GraphicDriver(nullptr),
    m_WindowResized(false)
{
}

WindowsApplication::~WindowsApplication()
{
}

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
    m_GraphicDriver = new VulkanGraphicDriver();
    m_GraphicDriver->Initial();
    return 0;
}


static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<WindowsApplication*>(glfwGetWindowUserPointer(window));
    app->SetWindowResized(true);
}

int WindowsApplication::StartUp()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_MainWindow = glfwCreateWindow(WIDTH, HEIGHT, "Main window", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(m_MainWindow, framebufferResizeCallback);

    GraphicDriverErrorCode graphicErrorCode = m_GraphicDriver->StartUp(m_MainWindow);
    if (GraphicDriverErrorCode::OK!=graphicErrorCode)
    {
        return -1;
    }

    return 0;
}


int WindowsApplication::MainLoop()
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
    return 0;
}

void WindowsApplication::DrawFrame()
{
    m_GraphicDriver->DrawFrame();
}

void WindowsApplication::ResizeWindow()
{
    m_GraphicDriver->ResizeWindow();
}

int WindowsApplication::ShutDown()
{
    m_GraphicDriver->ShutDown();

    glfwDestroyWindow(m_MainWindow);
    m_MainWindow = nullptr;

    glfwTerminate();
    return 0;
}

void WindowsApplication::CleanUp()
{
    m_GraphicDriver->CleanUp();
    m_GraphicDriver = nullptr;
}
