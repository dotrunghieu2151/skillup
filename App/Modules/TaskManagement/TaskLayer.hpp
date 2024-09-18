#pragma once

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
  bool isLoadingTasks{false};
  bool isSavingTasks{false};
  bool open{false};
  std::jthread worker;
  Core::SpscQueue<std::unique_ptr<Core::Job>, 1> m_JobQueue{};

  void ProcessJob() {
    std::unique_ptr<Core::Job> j{nullptr};
    while (!m_JobQueue.Pop(j)) {
      std::this_thread::yield();
    }
    j->Execute();
  }

  void TriggerLoadTasks() {
    while (!m_JobQueue.Push(
        std::make_unique<LoadTaskJob>("tasks.bin", m, isLoadingTasks))) {
      std::this_thread::yield();
    }
    return;
  }

  void TriggerSaveTasks() {
    while (!m_JobQueue.Push(
        std::make_unique<SaveTaskJob>("tasks.bin", m, isSavingTasks))) {
      std::this_thread::yield();
    }
    return;
  }

public:
  std::vector<TaskGroup> m{};
  std::shared_ptr<TaskGroupListUIComponent> taskGroupListUIComponent;
  TaskLayer()
      : taskGroupListUIComponent{std::make_shared<TaskGroupListUIComponent>(
            open, m)},
        worker(&TaskLayer::ProcessJob, this) {}

  void OnAttach() override {}

  void OnUIRender() override {
    if (open) {
      taskGroupListUIComponent->Render();
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
    Core::FileStreamWriter fWriter{"task.bin"};
    fWriter.WriteArray(m);
  }
};
} // namespace TaskManagement