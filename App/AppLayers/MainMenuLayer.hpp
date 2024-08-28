#pragma once

#include <iostream>

#include "Core/Application.hpp"
#include "TaskLayer.hpp"

class MainMenuLayer : public Core::ApplicationLayer {
private:
  std::shared_ptr<TaskLayer> m_TaskListUIComponent{};
  bool m_ShowDemoWindow{false};

public:
  void OnAwake() override {
    m_TaskListUIComponent = Core::Application::Get().GetLayer<TaskLayer>();
  }
  void OnUIRender() override {
    if (ImGui::BeginMainMenuBar()) {
      const ImGuiStyle& style = ImGui::GetStyle();
      ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_MenuBarBg]);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
      bool isTasksOpen = ImGui::Button("Tasks");
      ImGui::PopStyleColor();
      ImGui::PopStyleVar();

      if (isTasksOpen) {
        m_TaskListUIComponent->ToggleTaskList();
      }

      if (ImGui::BeginMenu("Config")) {
        ImGui::MenuItem("Demo window", NULL, &m_ShowDemoWindow);
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    if (m_ShowDemoWindow) {
      ImGui::ShowDemoWindow();
    }
  }
};