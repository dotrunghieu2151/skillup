#pragma once

#include <cassert>
#include <memory>
#include <string.h>
#include <utility>

namespace Core {
// a wrapper around a location of memory for easy read - write
// does not own the memory, thats up to the client
class Buffer {

private:
  bool m_IsAllocated{false};

public:
  void* m_Data;
  uint32_t m_Size;

  Buffer() : m_Data(nullptr), m_Size(0) {}
  Buffer(const void* data, uint32_t size) : m_Data{(void*)data}, m_Size{size} {}
  Buffer(const Buffer& other, uint32_t size)
      : m_Data(other.m_Data), m_Size(size) {}

  ~Buffer() {
    assert(!(m_IsAllocated && m_Data) &&
           "Buffer still holds data ! Need to Move or Free first");
  }

  void Allocate(uint32_t size) {
    assert(!m_Data && "Buffer still holds data ! Need to Move or Free first");

    if (size == 0)
      return;

    m_Data = new uint8_t[size];
    m_Size = size;
    m_IsAllocated = true;
  }

  void Free() {
    delete[] (uint8_t*)m_Data;
    m_Data = nullptr;
    m_IsAllocated = false;
    m_Size = 0;
  }

  std::pair<void*, uint32_t> Move() {
    std::pair<void*, uint32_t> p{m_Data, m_Size};

    m_Data = nullptr;
    m_Size = 0;
    m_IsAllocated = false;

    return p;
  }

  void ZeroInitialize() {
    if (m_Data) {
      memset(m_Data, 0, m_Size);
    }
  }

  template <typename T> T& Read(uint32_t offset = 0) {
    return *(T*)((uint32_t*)m_Data + offset);
  }

  template <typename T> const T& Read(uint32_t offset = 0) const {
    return *(T*)((uint32_t*)m_Data + offset);
  }

  uint8_t* CopyBytes(uint32_t size, uint32_t offset) const {
    assert(offset + size <= m_Size && "Buffer overflow!");
    uint8_t* buffer = new uint8_t[size];
    memcpy(buffer, (uint8_t*)m_Data + offset, size);
    return buffer;
  }

  void Write(const void* data, uint32_t size, uint32_t offset = 0) {
    assert(offset + size <= m_Size && "Buffer overflow!");
    memcpy((uint8_t*)m_Data + offset, data, size);
  }

  operator bool() const { return m_Data; }

  uint8_t& operator[](int index) { return ((uint8_t*)m_Data)[index]; }

  uint8_t& operator[](int index) const { return ((uint8_t*)m_Data)[index]; }

  template <typename T> T* As() const { return (T*)m_Data; }
};
} // namespace Core