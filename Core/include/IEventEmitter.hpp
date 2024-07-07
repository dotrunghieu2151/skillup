#pragma once

#include <functional>

namespace Core {
class IEventEmitter {
public:
  struct EventArgs {
    const IEventEmitter& sender;
  };

  virtual void On(const char*, std::function<void(const EventArgs&)>) = 0;

  virtual void Off(const char*, std::function<void(const EventArgs&)>) = 0;

  virtual void Trigger(const char*, const EventArgs&) = 0;
};
} // namespace Core