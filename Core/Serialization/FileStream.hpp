
#pragma once

#include "StreamReader.hpp"
#include "StreamWriter.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <string>

namespace Core {
//==============================================================================
/// FileStreamWriter
class FileStreamWriter : public StreamWriter {
public:
  FileStreamWriter(const std::filesystem::path& path);
  FileStreamWriter(const FileStreamWriter&) = delete;
  virtual ~FileStreamWriter();

  bool IsStreamGood() const final { return m_Stream.good(); }
  uint64_t GetStreamPosition() final { return m_Stream.tellp(); }
  void SetStreamPosition(uint64_t position) final { m_Stream.seekp(position); }
  bool WriteData(const char* data, uint32_t size) final;
  bool Flush() {
    m_Stream.flush();
    return m_Stream.fail();
  }

  static void AtomicWrite(const std::string& path,
                          std::function<void(FileStreamWriter&)> writeFn) {
    std::string tempName{"temp_" + path};
    {
      Core::FileStreamWriter writer{tempName};
      // atomic write to file:
      // write to temp
      writeFn(writer);
      // -> flush to OS
      // when Core::FileStreamWriter is destroyed, it automatically flush +
      // close stream so we don't need to do anything here
    }
    // -> rename / replace temp to path (we need to close the stream first)
    std::filesystem::rename(tempName, path);
    // -> delete temp
    // std::filesystem::rename auto deletes the new file if it exists/ the old
    // file is renamed to the new file so we don't need to do anything here
  }

private:
  std::filesystem::path m_Path;
  std::ofstream m_Stream;
};

//==============================================================================
/// FileStreamReader
class FileStreamReader : public StreamReader {
public:
  FileStreamReader(const std::filesystem::path& path);
  FileStreamReader(const FileStreamReader&) = delete;
  ~FileStreamReader();

  bool IsStreamGood() const final { return m_Stream.good(); }
  uint64_t GetStreamPosition() override { return m_Stream.tellg(); }
  void SetStreamPosition(uint64_t position) override {
    m_Stream.seekg(position);
  }
  bool ReadData(char* destination, uint32_t size) override;

private:
  std::filesystem::path m_Path;
  std::ifstream m_Stream;
};

} // namespace Core
