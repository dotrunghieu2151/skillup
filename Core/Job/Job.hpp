#pragma once

namespace Core {
class Job {
public:
  virtual ~Job() = default;

  virtual void Execute() = 0;
};
} // namespace Core