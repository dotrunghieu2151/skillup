add_library(Transcribe STATIC)

target_sources(Transcribe 
  PRIVATE
    WhisperTranscriber.cpp
    # MarinaTranslator.cpp
    AudioFileWriter.hpp
)

target_include_directories(Transcribe PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# Link required libraries
target_link_libraries(Transcribe 
  PUBLIC 
    Core
    whisper
    soxr::soxr
    portaudio
    # llama
)