#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

struct ShaderProgramSource {
  std::string VertextSource;
  std::string FragmentSource;
};

class Shader {
private:
  std::string m_Filepath;
  unsigned int m_RendererID;

  std::unordered_map<std::string, int> m_UniformLocationCache;
  // cache for uniforms

public:
  Shader(const std::string& filepath);
  ~Shader();

  void Bind() const;
  void Unbind() const;

  // set uniform
  void SetUniform1i(const std::string& name, int value);
  void SetUniform4f(const std::string& name, float v0, float v1, float v2,
                    float v3);
  void SetUniformMat4f(const std::string& name, glm::mat4 value);

private:
  ShaderProgramSource ParseShader(const std::string& filepath);
  int GetUniformLocation(const std::string& name);
  unsigned int CreateShader(const std::string& vertexShader,
                            const std::string& fragmentShader);
  unsigned int CompileShader(unsigned int type, const std::string& source);
};