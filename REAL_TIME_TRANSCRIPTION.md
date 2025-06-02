# Real-time Audio Transcription & Translation

This document describes the real-time audio transcription and translation system integrated into the SkillUp application.

## Features

### 🎤 Real-time Audio Transcription
- **Live Japanese Speech Recognition**: Transcribes spoken Japanese in real-time using Whisper.cpp
- **Voice Activity Detection (VAD)**: Automatically detects speech and filters out silence
- **GPU Acceleration**: Utilizes NVIDIA CUDA for fast processing
- **Audio Context Management**: Maintains 10 seconds of audio context for better accuracy

### 🌐 Real-time Translation
- **Japanese to English Translation**: Converts transcribed Japanese text to English using Whisper's built-in translation
- **Dual Processing**: Simultaneous transcription and translation pipelines
- **Neural Translation**: High-quality translation using Whisper's multilingual model
- **Real-time Processing**: Live translation with minimal latency

### 🎛️ Advanced Audio Processing
- **Multi-device Support**: Works with any WASAPI-compatible audio input device
- **Configurable Parameters**: Adjustable VAD threshold, segment length, and overlap
- **Audio Visualization**: Real-time waveform display
- **Context Preservation**: Maintains audio history for improved transcription accuracy

## How It Works

### Audio Pipeline
1. **Audio Capture**: Records audio from selected input device at 48kHz
2. **Context Management**: Maintains rolling buffer of last 10 seconds
3. **VAD Processing**: Detects speech segments using RMS-based analysis
4. **Dual Processing**: Sends audio to both transcription and translation pipelines

### Transcription Pipeline
```
Audio Input → VAD → Whisper (Japanese) → Japanese Text Output
```

### Translation Pipeline
```
Audio Input → VAD → Whisper (Japanese→English) → English Text Output
```

## Usage Instructions

### 1. Setup
- Ensure you have a microphone connected
- The application will automatically detect WASAPI-compatible devices
- GPU acceleration requires NVIDIA GPU with CUDA support

### 2. Starting Transcription
1. Open the "Real-time Transcription & Translation" window
2. Select your input device from the "Audio Devices" section
3. Enable "Auto-start transcription with recording" (recommended)
4. Click "🎤 Start Recording & Transcription"

### 3. Viewing Results
- **Japanese (Original)**: Shows real-time transcription of spoken Japanese
- **English (Translation)**: Shows translated text in English
- **Audio Waveform**: Visual representation of incoming audio

### 4. Controls
- **Pause**: Temporarily stops recording while maintaining transcription state
- **Stop**: Completely stops recording and transcription
- **Auto-start**: Automatically begins transcription when recording starts

## Configuration

### Whisper Settings
```cpp
// Main transcription (Japanese recognition)
config.model_path = "models/ggml-large-v3-turbo-q8_0.bin";  // Large turbo model for high accuracy
config.language = "ja";
config.translate_to_english = false;
config.segment_length_ms = 4000;  // 4-second segments for Japanese
config.overlap_ms = 750;          // 750ms overlap for better context
config.vad_threshold = 0.5f;      // Lower threshold for Japanese speech patterns
```

### Translation Settings
```cpp
// Translation pipeline (Japanese to English)
config.translate_to_english = true;  // Enable translation
config.n_threads = 2;               // Fewer threads for translation
config.segment_length_ms = 4000;    // Longer segments for better Japanese processing
```

## Translation Dictionary

The system includes a built-in dictionary for common English-Japanese translations:

### Greetings & Politeness
- hello → こんにちは
- goodbye → さようなら
- thank you → ありがとうございます
- please → お願いします
- excuse me → すみません

### Basic Questions
- what → 何
- where → どこ
- when → いつ
- why → なぜ
- how → どうやって

### Pronouns & Common Words
- I → 私
- you → あなた
- this → これ
- that → それ
- here → ここ
- there → そこ

### Time & Daily Life
- today → 今日
- tomorrow → 明日
- time → 時間
- work → 仕事
- school → 学校
- family → 家族

## Performance Optimization

### GPU Acceleration
- **CUDA Support**: Automatically uses NVIDIA GPU when available
- **Memory Management**: Efficient GPU memory usage for real-time processing
- **Parallel Processing**: Simultaneous transcription and translation

### Audio Processing
- **Buffer Management**: Circular buffer for continuous audio processing
- **Context Preservation**: Maintains audio history without memory leaks
- **VAD Optimization**: Reduces processing load by filtering silence

### Real-time Constraints
- **3-second Segments**: Balance between latency and accuracy
- **500ms Overlap**: Ensures no word boundaries are missed
- **Adaptive Threading**: Optimizes CPU usage for dual processing

## Technical Architecture

### Core Components

#### TranscribeLayer
- Main orchestrator for audio processing and transcription
- Manages dual Whisper instances for transcription and translation
- Handles audio context and buffer management

#### WhisperTranscriber
- Wrapper around whisper.cpp library
- Configurable for different languages and translation modes
- Thread-safe real-time processing

#### TranscribeUIComponent
- ImGui-based user interface
- Real-time display of transcription results
- Audio device selection and control

### Audio Flow
```
Microphone → PortAudio → AudioController → TranscribeLayer
                                              ↓
                                    ┌─────────────────┐
                                    │  Audio Context  │
                                    │   (10 seconds)  │
                                    └─────────────────┘
                                              ↓
                              ┌─────────────────────────────┐
                              │                             │
                    ┌─────────▼─────────┐         ┌─────────▼─────────┐
                    │ Whisper Instance 1│         │ Whisper Instance 2│
                    │   (Transcription) │         │   (Translation)   │
                    └─────────┬─────────┘         └─────────┬─────────┘
                              │                             │
                              ▼                             ▼
                    ┌─────────────────┐         ┌─────────────────────┐
                    │ English Text    │         │ Japanese Translation│
                    │ (Real-time)     │         │ (Word Mapping)      │
                    └─────────────────┘         └─────────────────────┘
```

## Extending the System

### Adding Translation Services
To integrate external translation APIs (Google Translate, Azure Translator):

```cpp
void TranslateToJapanese(const std::string& englishText) {
    // Replace the word mapping with API call
    std::string translation = CallTranslationAPI(englishText, "en", "ja");
    currentTranslation = translation;
}
```

### Supporting Additional Languages
1. Download appropriate Whisper models
2. Update language configuration
3. Add language-specific translation dictionaries

### Custom VAD Implementation
```cpp
bool HasSpeech(const float* samples, int sample_count) {
    // Implement custom voice activity detection
    // Consider spectral analysis, energy thresholds, etc.
    return CustomVADAlgorithm(samples, sample_count);
}
```

## Troubleshooting

### Common Issues

#### No Audio Input
- Check microphone permissions
- Verify device selection in audio devices list
- Ensure WASAPI drivers are installed

#### Poor Transcription Quality
- Adjust VAD threshold (0.4-0.8 range)
- Ensure clear audio input (reduce background noise)
- Check microphone positioning and quality

#### Translation Not Working
- Verify both Whisper instances are initialized
- Check translation dictionary for supported words
- Consider integrating external translation service

#### Performance Issues
- Reduce segment length for faster response
- Adjust thread count based on CPU cores
- Monitor GPU memory usage

### Debug Information
The application provides real-time status indicators:
- **Green dot (●)**: Transcription active
- **Status text**: Current transcription state
- **Audio waveform**: Visual feedback of audio input

## Future Enhancements

### Planned Features
1. **Multi-language Support**: Support for more input languages
2. **Cloud Translation**: Integration with Google Translate API
3. **Custom Models**: Support for domain-specific Whisper models
4. **Audio Recording**: Save transcribed audio sessions
5. **Export Functionality**: Export transcriptions to text files

### Performance Improvements
1. **Streaming VAD**: More sophisticated voice activity detection
2. **Model Optimization**: Quantized models for faster inference
3. **Batch Processing**: Optimize GPU utilization
4. **Memory Management**: Reduce memory footprint

## Dependencies

- **whisper.cpp**: Core transcription engine
- **PortAudio**: Cross-platform audio I/O
- **CUDA**: GPU acceleration (optional)
- **ImGui**: User interface framework

## License

This transcription system uses whisper.cpp under the MIT License. See the whisper.cpp repository for detailed license information. 