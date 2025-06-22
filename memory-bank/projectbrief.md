# SkillUp - Real-time Audio Transcription & Translation Application

## Project Overview

SkillUp is a C++20 desktop application that provides real-time audio transcription and translation capabilities, specifically designed for Japanese-English language processing. The application combines advanced AI models (Whisper.cpp for transcription, Llama.cpp for translation) with a modern ImGui-based user interface.

## Core Requirements

### Primary Functionality
1. **Real-time Audio Processing**
   - Capture audio from WASAPI-compatible devices
   - Process audio at 48kHz with VAD (Voice Activity Detection)
   - Maintain 10-second rolling audio context for accuracy

2. **AI-Powered Transcription**
   - Japanese speech recognition using Whisper.cpp models
   - GPU acceleration support (NVIDIA CUDA)
   - Real-time transcription with configurable parameters

3. **Translation Services**
   - Japanese-to-English translation using Whisper's built-in translator
   - Additional LLM-based translation via Marina Translator (Llama.cpp)
   - Dual translation pipelines for comparison

4. **User Interface**
   - ImGui-based modern interface with docking support
   - Real-time waveform visualization
   - Audio device selection and control
   - Transcription result display

### Technical Requirements
- **Language**: C++20
- **Build System**: CMake with vcpkg for dependencies
- **Platform**: Windows (primary), cross-platform design
- **Graphics**: OpenGL with GLFW
- **Audio**: PortAudio for device management

## Architecture Goals

### Modular Design
- **Core System**: Application framework, job system, streaming
- **App Modules**: Transcribe, TaskManagement, Decoder
- **Vendor Integration**: Whisper.cpp, Llama.cpp, ImGui

### Performance Targets
- Real-time processing (< 3 second latency)
- GPU acceleration for AI models
- Efficient memory management for continuous operation

### Scalability
- Plugin architecture for additional AI models
- Extensible translation services
- Configurable processing parameters

## Success Criteria

1. **Functional**: Successful real-time Japanese transcription with >90% accuracy
2. **Performance**: Sub-3 second transcription latency on modern hardware
3. **Usability**: Intuitive interface for non-technical users
4. **Reliability**: Stable operation for extended recording sessions
5. **Extensibility**: Easy integration of new AI models and languages

## Development Constraints

- **Memory**: Efficient GPU memory usage for large AI models
- **Latency**: Real-time processing requirements
- **Dependencies**: Minimal external dependencies beyond core libraries
- **Compatibility**: Support for various audio devices and GPU configurations 