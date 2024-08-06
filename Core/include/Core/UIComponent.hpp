#pragma once

namespace Core {

// An UI component that preserves states between render
class UIStatefulComponent {
public:
  virtual ~UIStatefulComponent() = default;

  virtual void Render() = 0;
};
} // namespace Core