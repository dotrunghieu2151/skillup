#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <imgui.h>

#include "ApplicationLayer.hpp"

struct GLFWwindow;

namespace Core {
struct ApplicationSpecification {
  std::string Name = "Application";
  uint32_t Width = 1600;
  uint32_t Height = 900;

  bool WindowResizeable = true;

  // Uses custom Walnut titlebar instead
  // of Windows default
  // bool CustomTitlebar = false;

  // Window will be created in the center
  // of primary monitor
  bool CenterWindow = false;
};

class Application {
public:
  Application(const ApplicationSpecification& applicationSpecification =
                  ApplicationSpecification{});

  ~Application();

  void Run();
  void Close();

  void SetMenubarCallback(const std::function<void()>& menubarCallback) {
    m_MenubarCallback = menubarCallback;
  }

  template <std::derived_from<ApplicationLayer> T> void PushLayer() {
    m_LayerList.emplace_back(std::make_shared<T>())->OnAttach();
  }

  template <std::derived_from<ApplicationLayer> T>
  void PushLayer(const std::string& id) {
    auto insert = m_LayerList.emplace_back(std::make_shared<T>());
    insert->SetId(id);
    insert->OnAttach();
  }

  void PushLayer(const std::shared_ptr<ApplicationLayer>& layer) {
    m_LayerList.emplace_back(layer);
    layer->OnAttach();
  }

  template <std::derived_from<ApplicationLayer> T>
  std::shared_ptr<T> GetLayer() {
    for (std::shared_ptr<ApplicationLayer> p : m_LayerList) {
      std::shared_ptr<T> layer{std::dynamic_pointer_cast<T>(p)};
      if (layer) {
        return layer;
      }
    }

    return nullptr;
  }

  template <std::derived_from<ApplicationLayer> T>
  std::shared_ptr<T> GetLayer(const std::string& id) {
    for (std::shared_ptr<ApplicationLayer> p : m_LayerList) {
      std::shared_ptr<T> layer{std::dynamic_pointer_cast<T>(p)};
      if (layer && layer->GetId() == id) {
        return layer;
      }
    }

    return nullptr;
  }

  static Application& Get();
  static ImFont* GetFont(const std::string& name);

  bool IsMaximized() const;
  float GetTime();
  GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }
  bool IsTitleBarHovered() const { return m_TitleBarHovered; }

private:
  ApplicationSpecification m_Specification;
  GLFWwindow* m_WindowHandle = nullptr;
  bool m_Running = false;

  float m_TimeStep = 0.0f;
  float m_FrameTime = 0.0f;
  float m_LastFrameTime = 0.0f;

  bool m_TitleBarHovered = false;

  std::vector<std::shared_ptr<ApplicationLayer>> m_LayerList;
  std::function<void()> m_MenubarCallback;

  void Init();
  void Shutdown();
};
} // namespace Core