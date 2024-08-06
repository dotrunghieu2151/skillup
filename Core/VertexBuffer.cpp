#include "Core/VertexBuffer.hpp"
#include "Core/Renderer.hpp"

VertexBuffer::VertexBuffer(const void* data, unsigned int size) {
  // create buffer
  glGenBuffers(1, &m_RendererID);

  // select buffer, passing the type of buffer and buffer id
  glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);

  // specify the size of the buffer, by passing the data now, or maybe later ;)
  glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer() { glDeleteBuffers(1, &m_RendererID); }

void VertexBuffer::Bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_RendererID); }

void VertexBuffer::Unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }