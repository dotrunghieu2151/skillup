#pragma once

namespace Core {
class ApplicationLayer {
public:
  virtual ~ApplicationLayer() = default;

  virtual void OnAttach() {}
  virtual void OnDetach() {}

  virtual void OnUpdate(float deltaTime) {}
  virtual void OnUIRender() {}
};
} // namespace Core