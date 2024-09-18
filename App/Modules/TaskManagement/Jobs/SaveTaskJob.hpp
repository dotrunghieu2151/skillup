#pragma once

#include <string>

#include "Core/Job/Job.hpp"
#include "Core/Serialization/FileStream.hpp"

namespace TaskManagement {
class SaveTaskJob : public Core::Job {
private:
  bool& m_IsSaving;
  Core::FileStreamWriter m_Writer;
  std::vector<TaskGroup> m_Data;

public:
  SaveTaskJob(const std::string& path, const std::vector<TaskGroup>& data,
              bool& isSaving)
      : m_Writer{path}, m_Data{data}, m_IsSaving{isSaving} {}

  void Execute() override {
    m_IsSaving = true;
    m_Writer.WriteArray(m_Data);
    m_IsSaving = false;
  }
};
} // namespace TaskManagement