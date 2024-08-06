#pragma once

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "Core/Event.hpp"
#include "Core/EventEmitter.hpp"
#include "Core/UIComponent.hpp"
#include "Core/UtilComponent.hpp"

struct TaskUIItem {
  std::string title{};
  std::string description{};
  float progress{0.0f};
  bool checked{false};
};

class TaskListUIComponent : public Core::UIStatefulComponent {
public:
  // struct EventTask : Core::EventSystem::Event<TaskListUIComponent> {
  //   bool hi;
  // };

  // inline Core::EventSystem::EventEmitter<EventTask>& OnInitEvent() {
  //   return m_OnInitEvent;
  // }

  TaskListUIComponent(bool& visible, std::vector<TaskUIItem>& items)
      : m_Visible{visible}, m_Items{items} {}

  void Render() override {
    if (!ImGui::Begin("Task list", &m_Visible,
                      ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::End();
      return;
    }

    for (int i{}; i < m_Items.size(); ++i) {
      TaskUIItem& item{m_Items[i]};
      ImGui::BeginGroup();
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 5.0f));
      bool showThisTask = ImGui::TreeNodeEx(
          std::format("{}###TaskItem_Title_{}_Text", item.title, i).c_str(),
          ImGuiTreeNodeFlags_OpenOnArrow |
              ImGuiTreeNodeFlags_OpenOnDoubleClick |
              ImGuiTreeNodeFlags_SpanTextWidth |
              ImGuiTreeNodeFlags_FramePadding);
      ImGui::PopStyleVar();
      ImGui::SameLine();
      if (ImGui::Checkbox(std::format("##TaskItem_{}_Checkbox", i).c_str(),
                          &item.checked)) {
        if (item.checked) {
          item.progress = 1.0f;
        } else {
          item.progress = 0.0f;
        }
      }
      ImGui::SameLine();
      Core::UtilComponents::ProgressBarSlider(
          std::format("###TaskItem_ProgressBar_{}_Text", i), item.progress);
      if (item.progress == 1.0f) {
        item.checked = true;
      } else {
        item.checked = false;
      }
      if (showThisTask) {
        ImGui::InputText(std::format("##TaskItem_{}_Title", i).c_str(),
                         &item.title);
        ImGui::InputTextMultiline(
            std::format("##TaskItem_{}_Description", i).c_str(),
            &item.description);
        ImGui::TreePop();
      }
      ImGui::EndGroup();
    }

    ImGui::End();
  }

private:
  // Core::EventSystem::EventEmitter<EventTask> m_OnInitEvent{};
  std::vector<TaskUIItem>& m_Items;
  bool& m_Visible;
};