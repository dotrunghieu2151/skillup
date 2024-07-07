#pragma once

#include <algorithm>
#include <assert.h>
#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

#include "Event.hpp"
namespace Core {

namespace EventSystem {

template <typename C>
concept IsEvent = requires(C c) { []<typename X>(Event<X>&) {}(c); };

template <IsEvent EventType> class EventEmitter {
public:
  using FunctionType = std::shared_ptr<std::function<void(const EventType&)>>;

  void operator+=(FunctionType callback) { m_Listeners.push_back(callback); }

  void operator-=(FunctionType callback) {
    auto iter = std::find(m_Listeners.begin(), m_Listeners.end(), callback);
    if (iter == m_Listeners.end()) {
      return;
    }
    std::swap(*iter, m_Listeners.back());
    m_Listeners.pop_back();
  }

  void Trigger(const EventType& event) const {
    for (auto cbSharedPtr : m_Listeners) {
      (*cbSharedPtr)(event);
    }
  };

private:
  std::vector<FunctionType> m_Listeners{};
};
} // namespace EventSystem
} // namespace Core