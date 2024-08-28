#include "BufferStream.hpp"

namespace Core {
/// BufferStreamWriter
BufferStreamWriter::BufferStreamWriter(Buffer targetBuffer, uint64_t position)
    : m_TargetBuffer(targetBuffer), m_BufferPosition(position) {}
bool BufferStreamWriter::WriteData(const char* data, uint32_t size) {
  bool valid = m_BufferPosition + size <= m_TargetBuffer.m_Size;
  if (!valid)
    return false;

  m_TargetBuffer.Write(data, size, m_BufferPosition);
  m_BufferPosition += size;
  return true;
}

/// BufferStreamReader
BufferStreamReader::BufferStreamReader(Buffer targetBuffer, uint64_t position)
    : m_TargetBuffer(targetBuffer), m_BufferPosition(position) {}

bool BufferStreamReader::ReadData(char* destination, uint32_t size) {
  bool valid = m_BufferPosition + size <= m_TargetBuffer.m_Size;

  if (!valid)
    return false;

  memcpy(destination, m_TargetBuffer.As<uint8_t>() + m_BufferPosition, size);
  m_BufferPosition += size;
  return true;
}
} // namespace Core