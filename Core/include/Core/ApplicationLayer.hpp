#pragma once

#include "Application.hpp"
#include <string>
namespace Core {
class ApplicationLayer {
private:
  std::string m_Id{};

public:
  void SetId(const std::string& id) { m_Id = id; }

  inline const std::string& GetId() const { return m_Id; }
  virtual ~ApplicationLayer() = default;

  virtual void OnAttach() {}
  virtual void OnDetach() {}

  virtual void OnAwake() {}

  virtual void OnUpdate(float deltaTime) {}
  virtual void OnUIRender() {}
};
} // namespace Core