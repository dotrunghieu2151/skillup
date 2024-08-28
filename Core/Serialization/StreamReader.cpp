#include "StreamReader.hpp"

namespace Core {

bool StreamReader::ReadBuffer(Buffer& buffer) {

  if (buffer.m_Size == 0) {
    if (!ReadData((char*)&buffer.m_Size, sizeof(uint32_t))) {
      return false;
    }
    buffer.Allocate(buffer.m_Size);
  }

  return ReadData((char*)buffer.m_Data, buffer.m_Size);
}

bool StreamReader::ReadString(std::string& str) {
  size_t size;
  if (!ReadData((char*)&size, sizeof(size_t))) {
    return false;
  };

  str.resize(size);

  return ReadData((char*)str.data(), sizeof(char) * size);
}

template <>
void StreamReader::ReadArray(std::vector<std::string>& array, uint32_t size) {
  if (size == 0) {
    ReadRaw<uint32_t>(size);
  }

  array.resize(size);
  for (uint32_t i = 0; i < size; ++i) {
    ReadString(array[i]);
  }
}

} // namespace Core