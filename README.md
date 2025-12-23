# STM32F407 Real-Time Audio DSP System

Real-time audio processing system using STM32F407 Discovery board with overlap-add FFT-based spectral filtering.

## 🎯 Features

- **Real-time audio processing** at 48kHz sample rate
- **512-point FFT** with 50% overlap-add for frequency-domain filtering
- **Low-latency pipeline**: ~10.7ms total delay (512 samples)
- **Spectral filtering**: Configurable low-pass, high-pass, band-pass, and notch filters
- **Hardware acceleration**: CMSIS-DSP library optimized for ARM Cortex-M4F
- **DC offset removal**: Adaptive tracking for clean audio input

## 📋 Hardware Requirements

- **STM32F407VG Discovery Board**
- **Audio Input**: Analog signal to PA1 (ADC1_IN1)
- **Audio Output**: CS43L22 DAC via I2S3 (on-board)
- **External Components**: None required (uses on-board codec)

## 🔧 Technical Specifications

| Parameter | Value |
|-----------|-------|
| MCU | STM32F407VG (168MHz ARM Cortex-M4F) |
| Sample Rate | 48 kHz |
| ADC Resolution | 12-bit |
| DAC Resolution | 16-bit stereo |
| FFT Size | 512 samples |
| Hop Size | 256 samples (50% overlap) |
| Processing Latency | ~10.7 ms |
| Window Function | Hann (COLA-compliant) |

## 🏗️ System Architecture

```
Audio Input (PA1)(Initial version is on MAX4466)
    ↓
ADC1 (12-bit, DMA, Timer-triggered @ 48kHz)
    ↓
DC Offset Removal & Float Conversion
    ↓
Overlap-Add FFT Processing (512-point)
    ├─ Hann Windowing
    ├─ FFT (CMSIS-DSP)
    ├─ Spectral Mask Application
    └─ IFFT
    ↓
Float to Q15 Conversion
    ↓
I2S3 DMA (Stereo output)
    ↓
CS43L22 Codec → Headphones/Speakers
```

## 🚀 Getting Started

### Prerequisites

- **STM32CubeIDE** (or Keil MDK / IAR EWARM)
- **STM32CubeMX** (for hardware reconfiguration)
- **CMSIS-DSP Library** (included in STM32Cube)

### Quick Test

1. Connect audio source to **PA1** (ADC input)
2. Connect headphones to **3.5mm jack** on Discovery board
3. Power on the board
4. You should hear the processed audio output

## 🎛️ Configuration

### Enable Spectral Filtering

Edit `audio_processing.c` → `Init_Spectral_Mask()`:

**Low-pass filter (remove high frequencies above 4kHz):**
```c
float cutoff_freq = 4000.0f;
int cutoff_bin = (int)(cutoff_freq * FFT_SIZE / SAMPLE_RATE);
for (int k = cutoff_bin; k <= FFT_SIZE / 2; k++)
{
    spectral_mask[k] = 0.0f;
}
```

**Notch filter (remove 1kHz hum):**
```c
float notch_freq = 1000.0f;
int notch_bin = (int)(notch_freq * FFT_SIZE / SAMPLE_RATE);
int notch_width = 2;
for (int k = notch_bin - notch_width; k <= notch_bin + notch_width; k++)
{
    if (k >= 0 && k <= FFT_SIZE / 2)
        spectral_mask[k] = 0.0f;
}
```

**Voice band-pass (300Hz - 3400Hz):**
```c
float low_freq = 300.0f;
float high_freq = 3400.0f;
int low_bin = (int)(low_freq * FFT_SIZE / SAMPLE_RATE);
int high_bin = (int)(high_freq * FFT_SIZE / SAMPLE_RATE);
for (int k = 0; k <= FFT_SIZE / 2; k++)
{
    if (k < low_bin || k > high_bin)
        spectral_mask[k] = 0.0f;
}
```

### Adjust Output Gain

Edit `audio_processing.h`:
```c
#define AUDIO_GAIN  1.0f  // Change to desired gain (e.g., 1.5 for +3dB)
```

### Modify DC Offset Tracking Speed

Edit `audio_processing.h`:
```c
#define DC_ALPHA  0.01f  // Lower = slower tracking, more stable
                         // Higher = faster tracking, less stable
```

## 🧪 Testing

1. **Sweep test**: Input 20Hz-20kHz sine sweep, verify passband
2. **THD measurement**: Input 1kHz tone, measure harmonic distortion
3. **Latency test**: Input impulse, measure output delay with oscilloscope
4. **Filter verification**: Enable filters, verify frequency response with spectrum analyzer

## 🐛 Known Issues / Limitations

- **Startup transient**: Brief ~2ms pop during DC offset convergence
- **No anti-aliasing filter**: Input signals above 24kHz will alias
- **Fixed-point not used**: Float32 processing (could optimize for speed)
- **No adaptive filtering**: Spectral mask is static

## 🔮 Future Improvements

- [ ] Implement adaptive noise reduction (spectral subtraction)
- [ ] Add automatic gain control (AGC)
- [ ] Port to fixed-point Q15 for lower CPU usage
- [ ] Add USB audio class for PC connectivity
- [ ] Implement pitch shifting / time stretching

## 👤 Author

- GitHub: [@silanus23](https://github.com/silanus23)
- Email: berkantali23@outlook.com

## 🙏 Acknowledgments

- STMicroelectronics for HAL drivers and CMSIS-DSP
- ARM for Cortex-M4F DSP extensions and optimized math libraries
