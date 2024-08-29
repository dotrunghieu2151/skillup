#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Components/TaskGroupListUIComponent.hpp"
#include "Core/Application.hpp"
#include "Core/Serialization/FileStream.hpp"
#include "Entities/TaskItem.hpp"

class TaskLayer : public Core::ApplicationLayer {
public:
  bool open{false};
  std::vector<TaskGroup> m{
      // {"group 1",
      //  std::vector<TaskItem>{
      //      {TaskItem{
      //           "test", "despcriotn", 0.2f, false,
      //           std::vector<TaskItem>{{TaskItem{"test 1", "", 0.3f, false},
      //                                  TaskItem{"test 1.2", "", 0.4f,
      //                                  false}}}},
      //       TaskItem{"fda", "Da", 0.9f, true,
      //                std::vector<TaskItem>{
      //                    {TaskItem{"test 2", "", 0.6f, false},
      //                     TaskItem{"test 2.2", "", 0.8f, false}}}}}}},
      // {"group 2",
      //  std::vector<TaskItem>{
      //      {TaskItem{
      //           "test", "despcriotn", 0.2f, false,
      //           std::vector<TaskItem>{{TaskItem{"test 1", "", 0.3f, false},
      //                                  TaskItem{"test 1.2", "", 0.4f,
      //                                  false}}}},
      //       TaskItem{"fda", "Da", 0.9f, true,
      //                std::vector<TaskItem>{
      //                    {TaskItem{"test 2", "", 0.6f, false},
      //                     TaskItem{"test 2.2", "", 0.8f, false}}}}}}},
  };
  std::shared_ptr<TaskGroupListUIComponent> taskGroupListUIComponent;
  TaskLayer()
      : taskGroupListUIComponent{
            std::make_shared<TaskGroupListUIComponent>(open, m)} {}

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
    Core::FileStreamReader fReader{"task.bin"};
    fReader.ReadArray(m);
    OpenTaskList();
  }

  void SaveTaskList() {
    Core::FileStreamWriter fWriter{"task.bin"};
    fWriter.WriteArray(m);
  }
};