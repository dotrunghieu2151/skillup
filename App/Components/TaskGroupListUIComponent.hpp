#pragma once

#include <algorithm>
#include <cmath>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>
#include <imgui.h>
#include <imgui_stdlib.h>

#include "Core/Event.hpp"
#include "Core/EventEmitter.hpp"
#include "Core/UIComponent.hpp"
#include "Core/UtilComponent.hpp"
#include "Entities/TaskItem.hpp"
#include "UtilComponent.hpp"

class TaskGroupListUIComponent : public Core::UIStatefulComponent {
public:
  // struct EventTask : Core::EventSystem::Event<TaskListUIComponent> {
  //   bool hi;
  // };

  // inline Core::EventSystem::EventEmitter<EventTask>& OnInitEvent() {
  //   return m_OnInitEvent;
  // }

  TaskGroupListUIComponent(bool& visible, std::vector<TaskGroup>& items)
      : m_Visible{visible}, m_Items{items} {}

  void Render() override {
    if (!ImGui::Begin("Task list", &m_Visible,
                      ImGuiWindowFlags_AlwaysAutoResize |
                          ImGuiWindowFlags_MenuBar)) {
      ImGui::End();
      return;
    }
    if (ImGui::BeginMenuBar()) {
      if (ImGui::MenuItem("Add Group")) {
        m_OpenAddGroupPopup = true;
      };
      ImGui::EndMenuBar();
    }
    if (m_OpenAddGroupPopup) {
      ImGui::OpenPopup("Add Group?");
    }
    ShowAddGroupPopup();
    m_TextFilter.Draw("###TaskGroupFilter");
    Core::UtilComponents::TextCentered("Task Groups");
    for (int i{0}; i < m_Items.size(); ++i) {
      TaskGroup& tg{m_Items[i]};
      if (m_TextFilter.PassFilter(tg.title.c_str())) {
        if (ImGui::Button(fmt::format("{}###{}", tg.title.c_str(), i).c_str(),
                          ImVec2(0.0f, 0.0f))) {
          tg.open = !tg.open;
        }
        ImGui::SetItemTooltip("Right-click to delete");
        if (ImGui::BeginPopupContextItem()) // <-- use last item id as popup id
        {
          if (ImGui::Button("Delete")) {
            m_Items.erase(m_Items.begin() + i);
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            continue;
          }
          ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
          // Set payload to carry the index of our item (could be anything)
          ImGui::SetDragDropPayload("TG_DRAG_INDEX", &i, sizeof(int));

          ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload* payload =
                  ImGui::AcceptDragDropPayload("TG_DRAG_INDEX")) {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int*)payload->Data;
            TaskGroup temp{std::move(m_Items[payload_n])};
            m_Items.erase(m_Items.begin() + payload_n);
            m_Items.insert(m_Items.begin() + i, std::move(temp));
          }
          ImGui::EndDragDropTarget();
        }
      }
      if (tg.open) {
        TaskGroupUIComponent(
            fmt::format("{}###{}", tg.title.c_str(), i).c_str(), tg.subTasks,
            tg.open);
      }
    }
    ImGui::End();
  }

private:
  // Core::EventSystem::EventEmitter<EventTask> m_OnInitEvent{};
  std::vector<TaskGroup>& m_Items;
  ImGuiTextFilter m_TextFilter{};
  std::string m_TaskGroupNewName{};
  bool& m_Visible;
  bool m_OpenAddGroupPopup{false};

  void ShowAddGroupPopup() {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Add Group?", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::InputText("Group name", &m_TaskGroupNewName);
      if (ImGui::Button("OK", ImVec2(120, 0))) {
        if (m_TaskGroupNewName.size()) {
          m_Items.emplace_back(std::move(m_TaskGroupNewName));
        }
        m_OpenAddGroupPopup = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::SetItemDefaultFocus();
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        m_OpenAddGroupPopup = false;
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }
};