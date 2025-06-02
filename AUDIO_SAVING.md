# Audio Recording Save Feature

This document describes the audio file saving functionality in the SkillUp transcription system.

## Overview

The audio saving feature allows users to save their recorded audio sessions to WAV files for later playback, sharing, or archival purposes. The feature is seamlessly integrated into the transcription UI and only becomes available when there's recorded audio data.

## Features

### 🎵 WAV File Export
- **16-bit PCM WAV**: High compatibility format supported by all audio players
- **32-bit Float WAV**: Higher precision format for professional audio applications
- **Stereo Support**: Maintains 2-channel audio recording
- **48kHz Sample Rate**: Professional audio quality

### 💾 File Naming Options
- **Custom Filename**: User can specify their own filename
- **Auto-timestamped**: Automatically generates filename with current date/time
- **Directory Organization**: Saves files to `recordings/` directory

### 🎛️ User Interface
- **Conditional Visibility**: Save controls only appear when audio is recorded
- **Recording Information**: Shows duration and sample count
- **Real-time Feedback**: Immediate success/failure notification

## Usage Instructions

### 1. Recording Audio
1. Start the SkillUp application
2. Open the "Real-time Transcription & Translation" window
3. Select your input device
4. Click "🎤 Start Recording & Japanese Transcription"
5. Speak into your microphone
6. Click "Stop" when finished

### 2. Saving Audio Files

After recording, the save controls will appear in the "Recording Controls" section:

#### Manual Filename
1. Enter your desired filename in the text field
2. Click "💾 Save Audio File"
3. File will be saved as `recordings/[your_filename].wav`

#### Auto-timestamped Filename
1. Click "📅 Auto-name & Save"
2. File will be saved with timestamp: `recordings/recording_YYYYMMDD_HHMMSS.wav`

### 3. File Location
All saved audio files are stored in the `recordings/` directory relative to the application executable:
```
skillup.exe
└── recordings/
    ├── my_recording.wav
    ├── recording_20240315_143022.wav
    └── japanese_lesson_01.wav
```

## Technical Implementation

### AudioFileWriter Class
The core functionality is implemented in `AudioFileWriter.hpp`:

```cpp
class AudioFileWriter {
public:
  // Save as 16-bit PCM WAV (recommended for compatibility)
  static bool WriteWAV16(const std::string& filename, 
                         const std::vector<float>& samples,
                         int sample_rate, int channels = 2);
  
  // Save as 32-bit float WAV (higher precision)
  static bool WriteWAV(const std::string& filename, 
                       const std::vector<float>& samples,
                       int sample_rate, int channels = 2);
  
  // Generate timestamped filename
  static std::string GenerateTimestampedFilename(const std::string& prefix = "recording");
};
```

### Event System Integration
The save functionality uses the existing event system:

```cpp
// UI Component triggers save event
struct SaveAudioEvent : Core::EventSystem::Event<TranscribeUIComponent> {
  std::string filename;
};

// TranscribeLayer handles the event
transcribeUIComponent->OnSaveAudioEvent() += 
    [this](const SaveAudioEvent& event) {
      SaveAudioToFile(event.filename);
    };
```

### Audio Data Management
- Audio is stored in `std::vector<float>` format
- Real-time recording appends samples to the vector
- Save operation reads from the complete recording buffer
- Memory is managed automatically by the vector container

## File Formats

### 16-bit PCM WAV (Recommended)
- **Format**: PCM signed 16-bit
- **Sample Rate**: 48,000 Hz
- **Channels**: 2 (Stereo)
- **Bit Depth**: 16 bits
- **File Size**: ~11.5 MB per minute
- **Compatibility**: Universal support across all audio players

### 32-bit Float WAV (High Precision)
- **Format**: IEEE 754 32-bit float
- **Sample Rate**: 48,000 Hz
- **Channels**: 2 (Stereo)
- **Bit Depth**: 32 bits (float)
- **File Size**: ~23 MB per minute
- **Use Case**: Professional audio editing

## Error Handling

### Common Issues and Solutions

#### "Failed to save audio file"
- **Cause**: Insufficient disk space or write permissions
- **Solution**: Check available disk space and folder permissions

#### Save button not appearing
- **Cause**: No recorded audio data available
- **Solution**: Record some audio first, then stop recording

#### Empty or corrupted audio file
- **Cause**: Recording was stopped immediately or audio buffer was empty
- **Solution**: Ensure you record for at least a few seconds

### Debug Information
The application prints save operation results to the console:
```
Audio saved to: recordings/my_recording.wav
Duration: 15.23 seconds
```

## Performance Considerations

### Memory Usage
- Audio data is stored in RAM during recording
- 48kHz stereo: ~384 KB per second
- 10-minute recording: ~230 MB RAM usage
- Memory is freed when recording is cleared

### Disk Space
- 16-bit WAV: ~11.5 MB per minute
- 32-bit WAV: ~23 MB per minute
- Consider disk space for long recordings

### Save Performance
- File writing is performed synchronously
- Large recordings may take a few seconds to save
- UI remains responsive during save operation

## Future Enhancements

### Planned Features
1. **MP3 Export**: Compressed audio format for smaller file sizes
2. **Batch Export**: Save multiple recordings at once
3. **Metadata Embedding**: Include transcription text in audio file metadata
4. **Cloud Upload**: Direct upload to cloud storage services
5. **Audio Trimming**: Save only specific portions of recordings

### Format Support
1. **FLAC**: Lossless compression for archival
2. **OGG**: Open-source compressed format
3. **AAC**: Modern compressed format

## Troubleshooting

### Build Issues
If you encounter build errors related to audio saving:

1. Ensure C++17 or later is enabled (for `std::filesystem`)
2. Check that all includes are properly resolved
3. Verify filesystem permissions for the recordings directory

### Runtime Issues
For runtime problems:

1. Check console output for error messages
2. Verify the `recordings/` directory can be created
3. Ensure sufficient disk space is available
4. Test with shorter recordings first

## Code Examples

### Basic Usage
```cpp
// Check if audio is available for saving
if (transcribeLayer.HasRecordedAudio()) {
    float duration = transcribeLayer.GetRecordingDuration();
    std::cout << "Recording duration: " << duration << " seconds" << std::endl;
    
    // Save with custom filename
    transcribeLayer.SaveAudioToFile("my_recording.wav");
}
```

### Integration with UI
```cpp
// UI button handling
if (ImGui::Button("Save Recording")) {
    if (recordAudioData.size() > 0) {
        SaveAudioEvent event{"user_recording.wav"};
        transcribeUIComponent->OnSaveAudioEvent().Trigger(event);
    }
}
```

This audio saving feature enhances the transcription system by providing users with a complete audio workflow from recording to archival storage. 