# Technical Context - SkillUp Development

## Technology Stack

### Core Technologies
- **Language**: C++20 (requires C++20 for std::jthread, concepts)
- **Build System**: CMake 3.14+ with vcpkg integration
- **Package Manager**: vcpkg for dependency management
- **Platform**: Windows primary, designed for cross-platform

### Dependencies (vcpkg.json)
```json
{
  "fmt": "11.0.2+",           // String formatting
  "glew": "2.2.0#3+",         // OpenGL extensions
  "glfw3": "3.4+",            // Window management
  "glm": "1.0.1#2+",          // Mathematics library
  "imgui": "1.90.7+",         // UI framework with docking
  "portaudio": "19.7#5+",     // Audio I/O
  "soxr": "0.1.3#8+"          // Audio resampling
}
```

### AI Model Integration
- **Whisper.cpp**: Git submodule for speech recognition
- **Llama.cpp**: Git submodule for LLM-based translation
- **CUDA Support**: Optional GPU acceleration (NVIDIA only)

### Graphics and UI
- **OpenGL**: Graphics rendering backend
- **ImGui**: Immediate mode GUI with experimental docking
- **GLFW**: Cross-platform window and input management

## Development Environment

### Prerequisites
```bash
# Required tools
- CMake 3.14.0+
- vcpkg package manager
- Git (for submodules)
- Visual Studio 2022 or compatible C++20 compiler

# For GPU acceleration
- NVIDIA GPU with compute capability 6.0+
- CUDA Toolkit 11.0+
- cuDNN (optional, for performance)
```

### Build Configuration
```cmake
# CMake settings
CMAKE_CXX_STANDARD = 20
CMAKE_CXX_STANDARD_REQUIRED = YES

# Optimization flags
CMAKE_CXX_FLAGS_RELEASE = "-Ofast -march=native"

# Project structure
skillup/
├── App/           # Application layers
├── Core/          # Core systems
├── Vendors/       # Third-party code
├── Assets/        # Resources (models, fonts)
└── Tests/         # Unit tests
```

### Build Process
```bash
# Configure with vcpkg
cmake --preset vcpkg

# Build project
cmake --build out/build/vcpkg --config Release

# Run executable
./out/build/vcpkg/bin/Release/skillup.exe
```

## Audio Processing Architecture

### Audio Pipeline
```cpp
Hardware Microphone
  ↓ PortAudio (WASAPI on Windows)
  ↓ AudioController (48kHz, float32 samples)
  ↓ Core::Stream<float> (lock-free MPMC queue)
  ↓ VAD + Resampling (SoXR)
  ↓ AI Processing (Whisper/Llama models)
  ↓ Results Display (ImGui UI)
```

### Key Technical Specs
- **Sample Rate**: 48kHz (downsampled to 16kHz for Whisper)
- **Sample Format**: 32-bit float
- **Buffer Size**: Configurable, typically 512-1024 samples
- **Context Window**: 10 seconds of audio history
- **Processing Latency**: Target <3 seconds end-to-end

## AI Model Configuration

### Whisper Models
```cpp
// Model files in Assets/LLM/Whisper/
"ggml-large-v3-turbo-q8_0.bin"  // Primary Japanese model
"ggml-base.en.bin"              // Fallback English model

// Configuration
segment_length_ms: 4000         // 4-second processing segments
overlap_ms: 750                 // Overlap for context preservation
vad_threshold: 0.5              // Voice activity detection
```

### Llama Models (Marina Translator)
```cpp
// Model path
"Assets/LLM/Llama/webbigdata_gemma-2-2b-jpn-it-translate-gguf_gemma-2-2b-jpn-it-translate-Q4_K_M.gguf"

// Configuration
n_gpu_layers: 99                // Use maximum GPU acceleration
n_ctx: 2048                     // Context window size
sampler: greedy                 // Deterministic output
```

## Performance Considerations

### Memory Usage
- **GPU Memory**: 4-8GB for large Whisper models
- **System RAM**: 8-16GB recommended for smooth operation
- **Model Size**: Whisper large-v3 (~1.5GB), Llama 2B (~1.3GB)

### Threading Model
```cpp
Main Thread (UI)
├── Audio Thread (PortAudio callback)
├── Transcription Thread (Whisper processing)
├── Translation Thread (Llama processing)
└── Model Loading Thread (async initialization)
```

### Optimization Strategies
- **GPU Acceleration**: CUDA kernels for transformer operations
- **Lock-free Queues**: Audio processing without mutex contention
- **Batch Processing**: Group audio segments for efficient GPU utilization
- **Model Quantization**: Q4_K_M quantization for reduced memory usage

## Development Constraints

### Platform Limitations
- **Windows**: Primary target, full WASAPI audio support
- **CUDA**: NVIDIA GPUs only, fallback to CPU processing
- **Memory**: Large AI models require substantial GPU/RAM

### Technical Debt
- **Model Loading**: Synchronous loading blocks UI (improvement needed)
- **Error Handling**: Limited graceful degradation for model failures
- **Testing**: AI components difficult to unit test consistently

## Deployment Considerations

### Distribution
- **Executable**: Single executable with dynamic library dependencies
- **Models**: AI models distributed separately (~3GB total)
- **Runtime**: vcpkg redistributables and CUDA runtime

### System Requirements
```
Minimum:
- Windows 10 (64-bit)
- 8GB RAM
- 4GB disk space
- OpenGL 3.3 compatible GPU

Recommended:
- Windows 11 (64-bit)
- 16GB RAM
- NVIDIA GPU with 6GB+ VRAM
- SSD storage for model loading
- High-quality microphone
```

## Future Technical Directions

### Scalability
- **Model Swapping**: Hot-swap AI models without restart
- **Multi-language**: Support for additional language pairs
- **Cloud Integration**: Optional cloud-based translation APIs

### Performance
- **Streaming**: Real-time model inference optimization
- **Hardware**: Support for AMD GPU acceleration (ROCm)
- **Mobile**: Potential mobile deployment with model optimization 