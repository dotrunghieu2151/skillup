

#include <iostream>
#include <mutex>
#include <stdio.h>
#include <thread>
#include <unordered_map>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Application.hpp"

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id,
                            GLenum severity, GLsizei length,
                            const char *message, const void *userParam)
{
  // ignore non-significant error/warning codes
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
    return;

  std::cout << "---------------" << '\n';
  std::cout << "Debug message (" << id << "): " << message << '\n';

  switch (source)
  {
  case GL_DEBUG_SOURCE_API:
    std::cout << "Source: API";
    break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
    std::cout << "Source: Window System";
    break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER:
    std::cout << "Source: Shader Compiler";
    break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:
    std::cout << "Source: Third Party";
    break;
  case GL_DEBUG_SOURCE_APPLICATION:
    std::cout << "Source: Application";
    break;
  case GL_DEBUG_SOURCE_OTHER:
    std::cout << "Source: Other";
    break;
  }
  std::cout << '\n';

  switch (type)
  {
  case GL_DEBUG_TYPE_ERROR:
    std::cout << "Type: Error";
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    std::cout << "Type: Deprecated Behaviour";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    std::cout << "Type: Undefined Behaviour";
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    std::cout << "Type: Portability";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    std::cout << "Type: Performance";
    break;
  case GL_DEBUG_TYPE_MARKER:
    std::cout << "Type: Marker";
    break;
  case GL_DEBUG_TYPE_PUSH_GROUP:
    std::cout << "Type: Push Group";
    break;
  case GL_DEBUG_TYPE_POP_GROUP:
    std::cout << "Type: Pop Group";
    break;
  case GL_DEBUG_TYPE_OTHER:
    std::cout << "Type: Other";
    break;
  }
  std::cout << '\n';

  switch (severity)
  {
  case GL_DEBUG_SEVERITY_HIGH:
    std::cout << "Severity: high";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    std::cout << "Severity: medium";
    break;
  case GL_DEBUG_SEVERITY_LOW:
    std::cout << "Severity: low";
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    std::cout << "Severity: notification";
    break;
  }
  std::cout << '\n';
  std::cout << '\n';
}

static void glfw_error_callback(int error, const char *description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static Core::Application *s_Instance = nullptr;

static std::unordered_map<std::string, ImFont *> s_Fonts{};

namespace Core
{

  Application::Application(const ApplicationSpecification &specification)
      : m_Specification{specification}
  {
    s_Instance = this;

    Init();
  }

  Application::~Application()
  {
    Shutdown();

    s_Instance = nullptr;
  }

  Application &Application::Get() { return *s_Instance; }

  void Application::Init()
  {
    glfwSetErrorCallback(glfw_error_callback);
    assert(glfwInit());

    // opengl related
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // setup monitor
    GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *videoMode = glfwGetVideoMode(primaryMonitor);
    int monitorX, monitorY;
    glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    m_WindowHandle =
        glfwCreateWindow(m_Specification.Width, m_Specification.Height,
                         m_Specification.Name.c_str(), NULL, NULL);

    if (m_Specification.CenterWindow)
    {
      glfwSetWindowPos(m_WindowHandle,
                       monitorX + (videoMode->width - m_Specification.Width) / 2,
                       monitorY +
                           (videoMode->height - m_Specification.Height) / 2);

      glfwSetWindowAttrib(m_WindowHandle, GLFW_RESIZABLE,
                          m_Specification.WindowResizeable ? GLFW_TRUE
                                                           : GLFW_FALSE);
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(m_WindowHandle);

    std::cout << glGetString(GL_VERSION) << '\n';
    // sync with monitor refresh rate
    glfwSwapInterval(1);

    glfwShowWindow(m_WindowHandle);

    glfwSetWindowUserPointer(m_WindowHandle, this);

    // glfwSetTitlebarHitTestCallback(
    //     m_WindowHandle, [](GLFWwindow* window, int x, int y, int* hit) {
    //       Application* app = (Application*)glfwGetWindowUserPointer(window);
    //       *hit = app->IsTitleBarHovered();
    //     });

    // opengl related
    if (glewInit() != GLEW_OK)
    {
      std::cout << "GLEW NOT OK" << '\n';
    }

    // enable debugging
    int openGlDebugEnableflags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &openGlDebugEnableflags);
    if (openGlDebugEnableflags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
      // initialize debug output
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageCallback(glDebugOutput, NULL);
      // for filtering errors
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                            GL_TRUE);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;              // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport /
                                                        // Platform Windows
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);
    style.ChildRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform
    // windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      style.WindowRounding = 0.0f;
      style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    ImGui_ImplGlfw_InitForOpenGL(m_WindowHandle, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Load default font
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    ImFont *robotoFont = io.Fonts->AddFontFromFileTTF(
        "Assets/Fonts/Roboto-Regular.ttf", 20.0f, &fontConfig);
    s_Fonts["Default"] = robotoFont;
    s_Fonts["Bold"] = io.Fonts->AddFontFromFileTTF("Assets/Fonts/Roboto-Bold.ttf",
                                                   20.0f, &fontConfig);
    s_Fonts["Italic"] = io.Fonts->AddFontFromFileTTF(
        "Assets/Fonts/Roboto-Italic.ttf", 20.0f, &fontConfig);
    io.FontDefault = robotoFont;
  }

  void Application::Run()
  {
    m_Running = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO &io = ImGui::GetIO();
    while (!glfwWindowShouldClose(m_WindowHandle) && m_Running)
    {
      assert(glGetError() == GL_NO_ERROR);

      glfwPollEvents();

      {
        std::scoped_lock<std::mutex> lock(m_EventQueueMutex);

        // Process custom event queue
        while (m_EventQueue.size() > 0)
        {
          auto &func = m_EventQueue.front();
          func();
          m_EventQueue.pop();
        }
      }

      // update layers
      for (auto &layer : m_LayerList)
      {
        layer->OnUpdate(m_TimeStep);
      }

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

      for (auto &layer : m_LayerList)
      {
        layer->OnUIRender();
      }

      ImGui::Render();
      ImDrawData *main_draw_data = ImGui::GetDrawData();

      const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f ||
                                      main_draw_data->DisplaySize.y <= 0.0f);
      int display_w, display_h;
      glfwGetFramebufferSize(m_WindowHandle, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClear(GL_COLOR_BUFFER_BIT);
      if (!main_is_minimized)
      {
        ImGui_ImplOpenGL3_RenderDrawData(main_draw_data);
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }

      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
      {
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
      }

      /* Swap front and back buffers */
      glfwSwapBuffers(m_WindowHandle);

      float time = GetTime();
      m_FrameTime = time - m_LastFrameTime;
      m_TimeStep = glm::min<float>(m_FrameTime, 0.0333f);
      m_LastFrameTime = time;
    }
  }

  void Application::Close() { m_Running = false; }

  void Application::Shutdown()
  {
    for (auto &layer : m_LayerList)
    {
      layer->OnDetach();
    }

    m_LayerList.clear();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_WindowHandle);
    glfwTerminate();
  }

  bool Application::IsMaximized() const
  {
    return (bool)glfwGetWindowAttrib(m_WindowHandle, GLFW_MAXIMIZED);
  }

  float Application::GetTime() { return (float)glfwGetTime(); }

  ImFont *Application::GetFont(const std::string &name)
  {
    if (!s_Fonts.contains(name))
      return nullptr;

    return s_Fonts.at(name);
  }
} // namespace Core
