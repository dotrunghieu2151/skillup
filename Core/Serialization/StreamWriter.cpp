#include "StreamWriter.hpp"

namespace Core {

void StreamWriter::WriteBuffer(Buffer buffer, bool writeSize) {
  if (writeSize)
    WriteData((char*)&buffer.m_Size, sizeof(uint32_t));

  WriteData((char*)buffer.m_Data, buffer.m_Size);
}

void StreamWriter::WriteZero(uint32_t size) {
  char zero = 0;
  for (uint64_t i = 0; i < size; i++)
    WriteData(&zero, 1);
}

void StreamWriter::WriteString(const std::string& string) {
  size_t size = string.size();
  WriteData((char*)&size, sizeof(size_t));
  WriteData((char*)string.data(), sizeof(char) * string.size());
}

void StreamWriter::WriteString(std::string_view string) {
  size_t size = string.size();
  WriteData((char*)&size, sizeof(size_t));
  WriteData((char*)string.data(), sizeof(char) * string.size());
}

template <>
void StreamWriter::WriteArray(const std::vector<std::string>& array,
                              bool writeSize) {
  if (writeSize) {
    WriteRaw<uint32_t>((uint32_t)array.size());
  }

  for (const auto& element : array) {
    WriteString(element);
  }
}

} // namespace Core