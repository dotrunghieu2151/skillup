#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Core/Serialization/StreamReader.hpp"
#include "Core/Serialization/StreamWriter.hpp"

struct TaskItem {
  std::string title{};
  std::string description{};
  float progress{0.0f};
  bool checked{false};

  std::vector<TaskItem> subTasks{0};

  static void Serialize(Core::StreamWriter* stream, const TaskItem& instance) {
    stream->WriteString(instance.title);
    stream->WriteString(instance.description);
    stream->WriteRaw(instance.progress);
    stream->WriteRaw(instance.checked);
    stream->WriteArray(instance.subTasks);
  }

  static void Deserialize(Core::StreamReader* stream, TaskItem& instance) {
    stream->ReadString(instance.title);
    stream->ReadString(instance.description);
    stream->ReadRaw(instance.progress);
    stream->ReadRaw(instance.checked);
    stream->ReadArray(instance.subTasks);
  }
};

struct TaskGroup {
  std::string title{};
  std::vector<TaskItem> subTasks{0};
  bool open{false};

  static void Serialize(Core::StreamWriter* stream, const TaskGroup& instance) {
    stream->WriteString(instance.title);
    stream->WriteArray(instance.subTasks);
    stream->WriteRaw(instance.open);
  }

  static void Deserialize(Core::StreamReader* stream, TaskGroup& instance) {
    stream->ReadString(instance.title);
    stream->ReadArray(instance.subTasks);
    stream->ReadRaw(instance.open);
  }
};