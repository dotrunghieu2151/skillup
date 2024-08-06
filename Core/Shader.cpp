#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Core/Renderer.hpp"
#include "Core/Shader.hpp"

Shader::Shader(const std::string& filepath)
    : m_Filepath{filepath}, m_RendererID{0} {
  ShaderProgramSource shaderSources{ParseShader(filepath)};
  m_RendererID =
      CreateShader(shaderSources.VertextSource, shaderSources.FragmentSource);
}

Shader::~Shader() { glDeleteProgram(m_RendererID); }

void Shader::Bind() const { glUseProgram(m_RendererID); }

void Shader::Unbind() const { glUseProgram(0); }

void Shader::SetUniform1i(const std::string& name, int value) {
  glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2,
                          float v3) {
  glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
}

void Shader::SetUniformMat4f(const std::string& name, glm::mat4 value) {
  glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &value[0][0]);
};

int Shader::GetUniformLocation(const std::string& name) {
  const auto findName = m_UniformLocationCache.find(name);
  if (findName != m_UniformLocationCache.end()) {
    return findName->second;
  }

  int uniformVariableId = glGetUniformLocation(m_RendererID, name.c_str());
  m_UniformLocationCache[name] = uniformVariableId;
  return uniformVariableId;
}

ShaderProgramSource Shader::ParseShader(const std::string& filepath) {
  std::ifstream stream{filepath};

  enum class ShaderType {
    NONE = -1,
    VERTEX = 0,
    FRAGMENT = 1,
  };

  ShaderType type = ShaderType::NONE;
  std::string line;
  std::stringstream ss[2];
  while (getline(stream, line)) {
    if (line.find("#shader") != std::string::npos) {
      if (line.find("vertex") != std::string::npos) {
        type = ShaderType::VERTEX;
      } else if (line.find("fragment") != std::string::npos) {
        type = ShaderType::FRAGMENT;
      }
    } else {
      ss[(int)type] << line << '\n';
    }
  }

  return {ss[0].str(), ss[1].str()};
}

unsigned int Shader::CompileShader(unsigned int type,
                                   const std::string& source) {
  unsigned int id = glCreateShader(type);
  const char* src = source.c_str();
  glShaderSource(id, 1, &src, nullptr);
  glCompileShader(id);

  // todo handle errors
  int result;
  glGetShaderiv(id, GL_COMPILE_STATUS, &result);
  if (!result) {
    int length;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
    char* message = (char*)alloca(length * sizeof(char));
    glGetShaderInfoLog(id, length, &length, message);
    std::cout << "Failed to compile "
              << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << '\n';
    std::cout << message << '\n';

    glDeleteShader(id);
    return 0;
  }
  return id;
}

unsigned int Shader::CreateShader(const std::string& vertexShader,
                                  const std::string& fragmentShader) {
  unsigned int program = glCreateProgram();

  unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
  assert(vs > 0);

  unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
  assert(fs > 0);

  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  glValidateProgram(program);

  glDeleteShader(vs);
  glDeleteShader(fs);

  return program;
}