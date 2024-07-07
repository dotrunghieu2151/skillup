#pragma once

namespace Core {
class UIComponent {
public:
  virtual ~UIComponent() = default;

  virtual void Render() = 0;
};
} // namespace Core