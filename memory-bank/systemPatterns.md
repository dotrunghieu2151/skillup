# System Patterns - SkillUp Architecture

## Core Architecture

### Application Framework
```cpp
Core::Application
├── ApplicationLayer (Abstract base)
├── MainMenuLayer (UI navigation)
├── TaskManagement::TaskLayer (Task system)
└── Transcribe::TranscribeLayer (Main functionality)
```

**Pattern**: Layer-based architecture with ImGui rendering
- Each layer handles specific domain functionality
- Event-driven communication between layers
- Shared application context for cross-layer coordination

### Stream Processing Architecture
```cpp
Audio Input → AudioController → Stream<float> → AI Processing → Results
```

**Pattern**: Producer-Consumer with buffering
- `Core::Stream<T>`: Lock-free MPMC queue for audio data
- Circular buffering for continuous audio processing
- Context preservation (10-second rolling window)

## AI Integration Patterns

### Whisper.cpp Integration
```cpp
WhisperTranscriber
├── Model Loading (whisper_init_from_file)
├── Real-time Processing (transcription thread)
├── Audio Resampling (SoXR integration)
└── Result Callbacks (non-blocking)
```

**Pattern**: Wrapped C API with RAII
- Safe C++ wrapper around whisper.cpp C API
- Thread-safe operation with atomic flags
- Automatic resource cleanup in destructor

### Llama.cpp Integration (Marina Translator)
```cpp
LlamaTranslator (nested struct)
├── Model Initialization (llama_model_load_from_file)
├── Context Management (llama_context)
├── Sampling Chain (llama_sampler_chain)
└── Token Processing (prompt_tokens, response)
```

**Pattern**: RAII wrapper with embedded configuration
- Encapsulated model lifecycle management
- GPU layer configuration (n_gpu_layers)
- Greedy sampling for deterministic output

## Component Communication

### Event System
```cpp
TranscribeUIComponent
├── OnRecordEvent (device selection, transcription enable)
├── OnPlaybackEvent (audio playback control)
├── OnPauseRecordEvent (pause/resume)
├── OnStopRecordEvent (complete stop)
└── OnSaveAudioEvent (file export)
```

**Pattern**: Event-driven architecture with callbacks
- Type-safe event emission using `Core::EventEmitter<T>`
- Lambda-based event handlers for loose coupling
- Immediate event processing in UI thread

### State Management
```cpp
TranscribeLayer State
├── Recording State (isRecording, recordingWithTranscription)
├── Processing State (isTranscribing, modelLoadingComplete)
├── Audio State (recordAudioData, maxFrequency)
└── AI State (whisperTranscriber, marinaTranslator)
```

**Pattern**: Centralized state with atomic flags
- Atomic booleans for thread-safe state queries
- State transitions managed in OnUpdate() cycle
- Clear separation of UI state vs processing state

## Memory Management Patterns

### Resource Lifecycle
```cpp
// RAII Pattern for AI Models
class WhisperTranscriber {
    whisper_context* m_Context;  // Managed in Initialize/Shutdown
    std::jthread transcriptionThread;  // Auto-joining thread
};

class MarinaTranslator {
    std::optional<LlamaTranslator> m_Translator;  // Optional ownership
};
```

**Pattern**: RAII with explicit initialization
- Explicit Initialize/Shutdown for heavy resources
- Optional<> for conditional resource ownership
- std::jthread for automatic thread cleanup

### Buffer Management
```cpp
Core::Stream<T>
├── Fixed-size circular buffer
├── Lock-free MPMC operations
└── Automatic overflow handling
```

**Pattern**: Lock-free circular buffer
- Fixed memory allocation at construction
- Atomic operations for thread safety
- Configurable buffer sizes based on use case

## Processing Patterns

### Dual AI Pipeline
```cpp
TranscribeLayer::ProcessAudioForTranscription()
├── WhisperTranscriber (Primary: Japanese → Japanese)
└── WhisperTranscriber (Secondary: Japanese → English)
```

**Pattern**: Parallel processing pipelines
- Same audio input processed by two AI models
- Different configurations for transcription vs translation
- Independent result callbacks for each pipeline

### Audio Context Management
```cpp
// Rolling context window
constexpr size_t MAX_CONTEXT_SAMPLES = 48000 * 10;  // 10 seconds
```

**Pattern**: Fixed-size context window
- Maintains audio history for better AI accuracy
- Prevents memory growth during long sessions
- Configurable context length based on model requirements

## Error Handling Patterns

### Graceful Degradation
```cpp
bool Initialize() {
    if (!LoadModel()) {
        std::cerr << "Failed to load model" << std::endl;
        return false;
    }
    // Continue with fallback behavior
}
```

**Pattern**: Explicit error propagation
- Boolean return values for initialization
- Fallback behavior when AI models unavailable
- User feedback through UI state changes

### Thread Safety
```cpp
std::atomic<bool> m_IsTranscribing{false};
std::jthread transcriptionThread;
```

**Pattern**: Atomic flags with stop tokens
- Atomic flags for thread-safe state queries
- std::stop_token for cooperative thread termination
- No shared mutable state between threads

## Configuration Patterns

### Structured Configuration
```cpp
struct Config {
    std::string model_path = "default/path";
    bool use_gpu = true;
    int n_threads = 4;
    float vad_threshold = 0.6f;
    // ... other parameters
};
```

**Pattern**: Aggregate initialization with defaults
- Struct-based configuration with sensible defaults
- Compile-time constant default values
- Easy to extend with new parameters

This architecture provides:
- **Modularity**: Clear separation of concerns
- **Extensibility**: Easy to add new AI models or languages
- **Performance**: Lock-free audio processing with GPU acceleration
- **Reliability**: RAII resource management and graceful error handling 