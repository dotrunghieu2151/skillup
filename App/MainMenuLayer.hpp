#pragma once

#include <iostream>

#include "Core/Application.hpp"
#include "Core/Queue/MpmcQueue.hpp"
#include "Core/Queue/SpscQueue.hpp"
#include "Modules/TaskManagement/TaskLayer.hpp"
#include "Modules/Transcribe/TranscribeLayer.hpp"

class MainMenuLayer : public Core::ApplicationLayer {
private:
  std::shared_ptr<TaskManagement::TaskLayer> m_TaskListUIComponent{nullptr};
  std::shared_ptr<Transcribe::TranscribeLayer> m_TranscribeLayer{nullptr};
  bool m_ShowDemoWindow{false};

public:
  void OnAwake() override {
    m_TaskListUIComponent =
        Core::Application::Get().GetLayer<TaskManagement::TaskLayer>();

    m_TranscribeLayer =
        Core::Application::Get().GetLayer<Transcribe::TranscribeLayer>();
  }
  void OnUIRender() override {
    if (ImGui::BeginMainMenuBar()) {
      const ImGuiStyle& style = ImGui::GetStyle();
      ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_MenuBarBg]);
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
      bool isTasksOpen = ImGui::Button("Tasks");
      bool isTaskSave = ImGui::Button("Save");
      bool isTaskLoad = ImGui::Button("Load");
      bool isTranscribe = ImGui::Button("Transcribe");
      ImGui::PopStyleColor();
      ImGui::PopStyleVar();

      if (isTasksOpen) {
        m_TaskListUIComponent->ToggleTaskList();
      }

      if (isTaskLoad) {
        m_TaskListUIComponent->LoadTaskList();
      }

      if (isTaskSave) {
        m_TaskListUIComponent->SaveTaskList();
      }

      if (isTranscribe) {
        m_TranscribeLayer->Toggle();
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