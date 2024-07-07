#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <type_traits>
#include <vector>

namespace Core {

template <typename Sender> struct Event {
  const Sender& sender;
};

template <typename EventType> class EventEmitter {
public:
  void operator+=(std::function<void(const EventType&)> callback) {
    m_Listeners.push_back(callback);
  }

  void operator-=(std::function<void(const EventType&)> callback) {
    auto iter = std::find(m_Listeners.begin(), m_Listeners.end(), callback);
    std::swap(*iter, m_Listeners.back());
    m_Listeners.pop_back();
  }

  void Trigger(const EventType& event) const {
    for (auto cb : m_Listeners) {
      cb(event);
    }
  };

private:
  std::vector<std::function<void(const EventType&)>> m_Listeners{};
};
} // namespace Core