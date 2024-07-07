#pragma once

namespace Core {
namespace EventSystem {
template <typename Sender> struct Event {
  const Sender& sender;
};
} // namespace EventSystem
} // namespace Core