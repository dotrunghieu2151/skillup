#pragma once

#include <vector>

#include "Components/TaskListUIComponent.hpp"
#include "Core/Application.hpp"

class TaskLayer : public Core::ApplicationLayer {
public:
  bool open{false};
  std::vector<TaskUIItem> v{{TaskUIItem{"test", "despcriotn", 0.2f, false},
                             TaskUIItem{"fda", "Da", 0.9f, true}}};
  std::shared_ptr<TaskListUIComponent> taskListUIComponent;
  TaskLayer()
      : taskListUIComponent{std::make_shared<TaskListUIComponent>(open, v)} {}

  void OnAttach() override {}

  void OnUIRender() override {
    if (open) {
      taskListUIComponent->Render();
    }
  }

  void OpenTaskList() { open = true; }

  void CloseTaskList() { open = false; }

  void ToggleTaskList() { open = !open; }
};