#pragma once

#include <algorithm>
#include <cmath>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include "Core/UtilComponent.hpp"
#include "Entities/TaskItem.hpp"

void TaskItemUIComponent(const std::string& id, TaskItem& task) {
  ImGui::BeginGroup();
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 5.0f));
  bool showThisTask = ImGui::TreeNodeEx(
      std::format("{}###{}", task.title, id).c_str(),
      ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
          ImGuiTreeNodeFlags_SpanTextWidth | ImGuiTreeNodeFlags_FramePadding);
  ImGui::PopStyleVar();
  if (ImGui::Checkbox((id + "_Checkbox").c_str(), &task.checked)) {
    if (task.checked) {
      task.progress = 1.0f;
    } else {
      task.progress = 0.0f;
    }
  }
  ImGui::SameLine();
  ImVec4 progressBarFillColor{0.90f, 0.70f, 0.00f, 1.00f};
  if (task.progress <= 0.3f) {
    progressBarFillColor = ImVec4{1.0f, 0.0f, 0.2f, 1.0f};
  } else if (task.progress == 1.0f) {
    progressBarFillColor = ImVec4{0.4f, 0.7f, 0.0f, 0.5f};
  }
  ImGui::PushStyleColor(ImGuiCol_PlotHistogram, progressBarFillColor);
  if (task.subTasks.size()) {
    ImGui::ProgressBar(task.progress, ImVec2(0.0f, 0.0f));
  } else {
    Core::UtilComponents::ProgressBarSlider((id + "_ProgressBarSlider").c_str(),
                                            task.progress);
  }
  ImGui::PopStyleColor();
  if (task.progress == 1.0f) {
    task.checked = true;
  } else {
    task.checked = false;
  }
  ImGui::EndGroup();
  ImGuiContext& g = *GImGui;

  ImGuiLastItemData lastItemData{g.LastItemData};
  if (showThisTask) {
    ImGui::Indent();
    ImGui::BeginGroup();
    ImGui::InputText((id + "_Title").c_str(), &task.title);
    ImGui::InputTextMultiline((id + "_Description").c_str(), &task.description);
    ImGui::EndGroup();

    ImGui::SetItemTooltip("Right-click to add task");
    if (ImGui::BeginPopupContextItem(
            std::format("###{}_add_task", id)
                .c_str())) // <-- use last item id as popup id
    {
      if (ImGui::Button("Add task")) {
        task.subTasks.push_back(TaskItem{});
      }
      ImGui::EndPopup();
    }

    if (task.subTasks.size()) {
      float totalProgress{0.0f};
      for (int i{}; i < task.subTasks.size(); ++i) {
        TaskItem& item{task.subTasks[i]};
        TaskItemUIComponent(id + "_st_" + std::to_string(i), item);
        ImGui::SetItemTooltip("Right-click to delete");
        if (ImGui::BeginPopupContextItem(
                std::format("###{}_{}", item.title, i)
                    .c_str())) // <-- use last item id as popup id
        {
          if (ImGui::Button("Delete")) {
            task.subTasks.erase(task.subTasks.begin() + i);
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            continue;
          }
          ImGui::EndPopup();
        }
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None |
                                       ImGuiDragDropFlags_SourceAllowNullID)) {
          // Set payload to carry the index of our item (could be anything)
          ImGui::SetDragDropPayload(id.c_str(), &i, sizeof(int));

          ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload* payload =
                  ImGui::AcceptDragDropPayload(id.c_str())) {
            IM_ASSERT(payload->DataSize == sizeof(int));
            int payload_n = *(const int*)payload->Data;
            std::swap(item, task.subTasks[payload_n]);
          }
          ImGui::EndDragDropTarget();
        }
        totalProgress += item.progress;
      }
      task.progress = totalProgress / task.subTasks.size();
    }
    ImGui::TreePop();
  }
  g.LastItemData = std::move(lastItemData);
};

void TaskGroupUIComponent(const std::string& taskGroupName,
                          std::vector<TaskItem>& taskList, bool& open) {
  if (!ImGui::Begin(taskGroupName.c_str(), &open,
                    ImGuiWindowFlags_AlwaysAutoResize |
                        ImGuiWindowFlags_MenuBar)) {
    ImGui::End();
    return;
  }

  if (ImGui::BeginMenuBar()) {
    if (ImGui::MenuItem("Add Task")) {
      taskList.push_back(TaskItem{});
    };
    ImGui::EndMenuBar();
  }

  for (int i{}; i < taskList.size(); ++i) {
    TaskItem& item{taskList[i]};

    TaskItemUIComponent("###TaskItem_" + std::to_string(i), item);
    ImGui::SetItemTooltip("Right-click to delete");
    if (ImGui::BeginPopupContextItem(
            std::format("###{}_{}", item.title, i)
                .c_str())) // <-- use last item id as popup id
    {
      if (ImGui::Button("Delete")) {
        taskList.erase(taskList.begin() + i);
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        continue;
      }
      ImGui::EndPopup();
    }
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None |
                                   ImGuiDragDropFlags_SourceAllowNullID)) {
      // Set payload to carry the index of our item (could be anything)
      ImGui::SetDragDropPayload("TaskItem_Drag_Index", &i, sizeof(int));

      ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload =
              ImGui::AcceptDragDropPayload("TaskItem_Drag_Index")) {
        IM_ASSERT(payload->DataSize == sizeof(int));
        int payload_n = *(const int*)payload->Data;
        std::swap(taskList[i], taskList[payload_n]);
      }
      ImGui::EndDragDropTarget();
    }
  }

  ImGui::End();
}