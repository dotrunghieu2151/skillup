#pragma once

#include <cstdio>
#include <functional>
#include <string>

#include "Core/Job/Job.hpp"
#include "Core/Serialization/FileStream.hpp"

namespace TaskManagement {
class SaveTaskJob : public Core::Job {
private:
  bool& m_IsSaving;
  std::string m_Path;

  std::vector<TaskGroup> m_Data;

public:
  SaveTaskJob(const std::string& path, const std::vector<TaskGroup>& data,
              bool& isSaving)
      : m_Path{path}, m_Data{data}, m_IsSaving{isSaving} {}

  void Execute() override {
    m_IsSaving = true;
    Core::FileStreamWriter::AtomicWrite(
        m_Path,
        [this](Core::FileStreamWriter& writer) { writer.WriteArray(m_Data); });
    m_IsSaving = false;
  }
};
} // namespace TaskManagement