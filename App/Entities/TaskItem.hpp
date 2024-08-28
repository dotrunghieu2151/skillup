#pragma once

#include <memory>
#include <string>
#include <vector>

struct TaskItem {
  std::string title{};
  std::string description{};
  float progress{0.0f};
  bool checked{false};

  std::vector<TaskItem> subTasks{0};
};

struct TaskGroup {
  std::string title{};
  std::vector<TaskItem> subTasks{0};
  bool open{false};
};