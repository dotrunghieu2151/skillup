#pragma once

#include <string>

#include "Core/Job/Job.hpp"
#include "Core/Serialization/FileStream.hpp"

namespace TaskManagement {
class LoadTaskJob : public Core::Job {
private:
  bool& m_IsLoading;
  Core::FileStreamReader m_Reader;
  std::vector<TaskGroup>& m_Data;

public:
  LoadTaskJob(const std::string& path, std::vector<TaskGroup>& data,
              bool& isLoading)
      : m_Reader{path}, m_Data{data}, m_IsLoading{isLoading} {}
  void Execute() override {
    m_IsLoading = false;
    m_Reader.ReadArray(m_Data);
    m_IsLoading = true;
  }
};
} // namespace TaskManagement