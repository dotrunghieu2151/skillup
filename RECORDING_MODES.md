# Recording Modes

This document describes the different recording modes available in the SkillUp transcription system, allowing users to choose between recording with real-time transcription or audio-only recording.

## Overview

The SkillUp application now offers two distinct recording modes to suit different use cases:

1. **Record with Transcription** - Full-featured mode with real-time Japanese transcription and English translation
2. **Record Audio Only** - Lightweight mode for pure audio recording without transcription overhead

## Recording Modes

### 🎯 Record with Transcription Mode

This is the full-featured mode that provides real-time transcription and translation capabilities.

**Features:**
- Real-time Japanese speech recognition
- Live English translation
- Voice Activity Detection (VAD)
- GPU-accelerated processing
- Audio context management
- Dual processing pipelines

**Best for:**
- Language learning sessions
- Meeting transcription
- Interview recording with transcripts
- Real-time translation needs
- Content creation with subtitles

**Resource Usage:**
- CPU: High (Whisper processing)
- GPU: High (if CUDA enabled)
- Memory: High (~230MB for 10 minutes)
- Battery: High impact on laptops

### 🎵 Record Audio Only Mode

This is a lightweight mode focused purely on high-quality audio recording without any transcription processing.

**Features:**
- High-quality 48kHz stereo recording
- Minimal CPU usage
- Real-time audio visualization
- Fast file saving
- Extended recording duration capability

**Best for:**
- Music recording
- Podcast recording
- Voice memos
- Long-duration recording sessions
- Battery-powered devices
- Quick audio notes

**Resource Usage:**
- CPU: Low (audio processing only)
- GPU: None
- Memory: Low (~23MB for 10 minutes)
- Battery: Minimal impact

## User Interface

### Mode Selection

The recording mode is selected in the "Recording Mode" section at the top of the transcription window:

```
Recording Mode
○ Record with Transcription    ○ Record Audio Only
  ☑ Auto-start transcription with recording
```

### Visual Indicators

The application provides clear visual feedback about the current recording state:

**Record with Transcription:**
- Recording & Transcribing: `● ●` (two green dots)
- Recording (transcription stopped): `● ○` (orange dot + empty circle)

**Record Audio Only:**
- Recording: `●` (single blue dot)

### Dynamic UI Sections

**Transcription Mode (Mode 0):**
- Shows "Live Transcription" section with Japanese and English text areas
- Button text: "🎤 Start Recording & Transcription"
- Auto-transcription checkbox available

**Audio Only Mode (Mode 1):**
- Shows "Audio Recording Info" section instead
- Button text: "🎤 Start Audio Recording"
- No transcription-related controls

## Usage Instructions

### Switching Recording Modes

1. Open the "Real-time Transcription & Translation" window
2. In the "Recording Mode" section, select your preferred mode:
   - **Record with Transcription**: For full transcription features
   - **Record Audio Only**: For pure audio recording
3. The UI will automatically adapt to show relevant controls

### Recording with Transcription

1. Select "Record with Transcription" mode
2. Choose your microphone from the input devices
3. Optionally enable "Auto-start transcription with recording"
4. Click "🎤 Start Recording & Transcription"
5. Speak in Japanese to see real-time transcription and translation
6. Click "Stop" when finished

### Recording Audio Only

1. Select "Record Audio Only" mode
2. Choose your microphone from the input devices
3. Click "🎤 Start Audio Recording"
4. Record your audio content
5. Monitor the waveform visualization for audio levels
6. Click "Stop" when finished

### Saving Recordings

Both modes support the same audio saving functionality:

- **Custom filename**: Enter your filename and click "💾 Save Audio File"
- **Auto-timestamped**: Click "📅 Auto-name & Save" for automatic naming

**Filename Prefixes:**
- Transcription mode: `transcribed_recording_YYYYMMDD_HHMMSS.wav`
- Audio only mode: `audio_recording_YYYYMMDD_HHMMSS.wav`

## Technical Implementation

### Mode Tracking

The recording mode is tracked through several components:

```cpp
struct RecordEvent {
  int inputDeviceID;
  bool enableTranscription; // New field for mode control
};

// UI Component
int m_RecordingMode{0}; // 0 = transcription, 1 = audio only

// TranscribeLayer  
bool recordingWithTranscription{true}; // Track current recording mode
```

### Conditional Processing

The audio processing pipeline adapts based on the recording mode:

```cpp
// Only process for transcription if recording with transcription enabled
if (recordingWithTranscription) {
  // Add to audio context for transcription
  AddToAudioContext(recordBuffer.buffer, recordBuffer.size);
  
  // Auto-start transcription if enabled and not already running
  if (autoStartTranscription && !isTranscribing) {
    StartTranscription();
  }
  
  // Process audio for real-time transcription
  ProcessAudioForTranscription(recordBuffer.buffer, recordBuffer.size);
}
```

### Memory Optimization

**Audio Only Mode Benefits:**
- No Whisper model loading (saves ~2GB GPU memory)
- No audio context buffering (saves ~230MB RAM for 10 minutes)
- No transcription result storage
- Faster startup time
- Lower CPU utilization

## Performance Comparison

| Aspect | Record with Transcription | Record Audio Only |
|--------|---------------------------|-------------------|
| Startup Time | ~5-10 seconds (model loading) | Instant |
| CPU Usage | High (60-80%) | Low (5-10%) |
| GPU Usage | High (if CUDA enabled) | None |
| Memory Usage | High (~2.5GB) | Low (~50MB) |
| Battery Impact | High | Minimal |
| Max Recording Duration | Limited by memory | Limited by disk space |
| File Processing | Real-time transcription | Audio encoding only |

## Use Case Examples

### Language Learning Session
**Mode:** Record with Transcription
**Benefit:** See immediate feedback on pronunciation and comprehension

### Podcast Recording
**Mode:** Record Audio Only  
**Benefit:** Minimal resource usage, focus on audio quality

### Meeting Notes
**Mode:** Record with Transcription
**Benefit:** Audio recording + automatic transcript generation

### Music Practice
**Mode:** Record Audio Only
**Benefit:** High-quality audio capture without interference

### Voice Memos
**Mode:** Record Audio Only
**Benefit:** Quick recording with minimal battery drain

## Troubleshooting

### Mode Selection Issues

**Problem:** Recording mode doesn't change
**Solution:** Stop any active recording before switching modes

**Problem:** Transcription doesn't start in transcription mode
**Solution:** Check if auto-start transcription is enabled

### Performance Issues

**Problem:** Laggy performance in transcription mode
**Solution:** Switch to audio-only mode for better performance

**Problem:** Audio dropouts during recording
**Solution:** Close other applications or use audio-only mode

### UI Display Issues

**Problem:** Transcription sections still visible in audio-only mode
**Solution:** Check if mode selection was properly applied

## Future Enhancements

### Planned Features

1. **Hybrid Mode**: Start in audio-only, add transcription post-recording
2. **Quality Presets**: Different audio quality settings for each mode
3. **Background Recording**: Record audio while using other applications
4. **Smart Mode**: Automatically switch based on available resources
5. **Export Options**: Different export formats per recording mode

### Advanced Options

1. **Compression Settings**: Choose audio compression for audio-only mode
2. **Channel Selection**: Mono/stereo options for different use cases
3. **Sample Rate Options**: Variable sample rates for different quality needs
4. **Real-time Effects**: Audio effects in audio-only mode

## Best Practices

### Choosing the Right Mode

**Use Transcription Mode when:**
- You need real-time feedback on speech
- Recording language learning content
- Need immediate translation
- Recording meetings or interviews
- Content requires subtitles

**Use Audio Only Mode when:**
- Recording music or high-quality audio
- Battery life is a concern  
- Recording for extended periods
- System resources are limited
- Focus is purely on audio quality

### Optimization Tips

1. **For Long Recordings**: Use audio-only mode to prevent memory issues
2. **For Real-time Feedback**: Use transcription mode with auto-start enabled
3. **For Battery Life**: Prefer audio-only mode on laptops
4. **For Quality**: Both modes use same high-quality 48kHz stereo recording

This dual-mode system provides flexibility for different recording needs while optimizing resource usage based on the specific requirements of each use case. 