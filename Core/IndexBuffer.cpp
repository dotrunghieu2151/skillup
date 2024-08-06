#include "Core/IndexBuffer.hpp"
#include "Core/Renderer.hpp"
#include <assert.h>

IndexBuffer::IndexBuffer(const unsigned int* data, unsigned int count)
    : m_Count{count} {
  assert(sizeof(unsigned int) == sizeof(GLuint));
  // create buffer
  glGenBuffers(1, &m_RendererID);

  // select buffer, passing the type of buffer and buffer id
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);

  // specify the size of the buffer, by passing the data now, or maybe later ;)
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data,
               GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer() { glDeleteBuffers(1, &m_RendererID); }

void IndexBuffer::Bind() const {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
}

void IndexBuffer::Unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }