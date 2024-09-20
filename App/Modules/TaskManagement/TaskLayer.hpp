#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Components/TaskGroupListUIComponent.hpp"
#include "Core/Application.hpp"
#include "Core/Job/Job.hpp"
#include "Core/Queue/SpscQueue.hpp"
#include "Entities/TaskItem.hpp"
#include "Jobs/LoadTaskJob.hpp"
#include "Jobs/SaveTaskJob.hpp"

namespace TaskManagement {
class TaskLayer : public Core::ApplicationLayer {
private:
  bool hasFirstLoaded{false};
  bool isLoadingTasks{false};
  bool isSavingTasks{false};
  bool open{false};
  Core::SpscQueue<std::unique_ptr<Core::Job>, 1> m_JobQueue{};
  std::jthread worker;

  void ProcessJob(std::stop_token s) {
    std::unique_ptr<Core::Job> j{nullptr};
    while (!s.stop_requested()) {
      if (m_JobQueue.Pop(j)) {
        j->Execute();
      } else {
        std::this_thread::yield();
      }
    }
  }

public:
  std::vector<TaskGroup> m{};
  std::shared_ptr<TaskGroupListUIComponent> taskGroupListUIComponent;
  TaskLayer()
      : taskGroupListUIComponent{std::make_shared<TaskGroupListUIComponent>(
            open, m, isLoadingTasks, isSavingTasks)},
        worker([this](std::stop_token s) { ProcessJob(s); }) {}

  void OnAttach() override {}

  void OnUpdate(float deltaTime) override {
    if (open && !isLoadingTasks && hasFirstLoaded && m_JobQueue.IsEmpty()) {
      m_JobQueue.Push(
          std::make_unique<SaveTaskJob>("tasks.bin", m, isSavingTasks));
    }
  }

  void OnUIRender() override {
    if (open) {
      if (!hasFirstLoaded) {
        m_JobQueue.Push(
            std::make_unique<LoadTaskJob>("tasks.bin", m, isLoadingTasks));
        hasFirstLoaded = true;
        isLoadingTasks = true;
      }
      taskGroupListUIComponent->Render();

      if (isSavingTasks) {
        ImVec2 vpSize = ImGui::GetMainViewport()->Size;
        ImGui::SetNextWindowPos(ImVec2(vpSize.x - 10.0f, vpSize.y + 40.0f),
                                ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::Begin("Notification", NULL,
                     ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                         ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoFocusOnAppearing);

        // We want to support multi-line text, this will
        // wrap the text after 1/3 of the screen width
        ImGui::PushTextWrapPos(vpSize.x / 3.f);
        ImGui::Text("Saving in progress...");
        ImGui::PopTextWrapPos();
        ImGui::End();
      }
    }
  }

  void OpenTaskList() { open = true; }

  void CloseTaskList() { open = false; }

  void ToggleTaskList() { open = !open; }

  void LoadTaskList() {
    // Core::FileStreamReader fReader{"task.bin"};
    // fReader.ReadArray(m);
    OpenTaskList();
  }

  void SaveTaskList() {
    Core::FileStreamWriter::AtomicWrite(
        "tasks.bin",
        [this](Core::FileStreamWriter& writer) { writer.WriteArray(m); });
  }
};
} // namespace TaskManagement