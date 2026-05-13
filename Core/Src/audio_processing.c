/**
  ******************************************************************************
  * @file    audio_processing.c
  * @brief   Real-time audio DSP processing implementation
  ******************************************************************************
  */

#include "audio_processing.h"
#include <string.h>
#include <math.h>

// Private buffers - aligned for optimal DMA/CMSIS-DSP performance
__attribute__((aligned(4))) static volatile float32_t fft_input[FFT_SIZE];
__attribute__((aligned(4))) static volatile float32_t fft_output[FFT_SIZE];
__attribute__((aligned(4))) static volatile float32_t overlap_buffer[HOP_SIZE];
__attribute__((aligned(4))) static volatile float32_t input_history[HOP_SIZE];
__attribute__((aligned(4))) static volatile float32_t window[FFT_SIZE];
__attribute__((aligned(4))) static volatile float32_t spectral_mask[FFT_SIZE / 2 + 1];


static arm_rfft_fast_instance_f32 fft_instance;
static float32_t dc_offset = 2048.0f;

static void Init_Hann_Window(void);
static void Init_Spectral_Mask(void);
static void Process_FFT_Block(float32_t *input, float32_t *output);
static void Apply_Spectral_Mask(float32_t *spectrum);

// Hann window: w[n] = 0.5 * (1 - cos(2*pi*n/(N-1))), reduces spectral leakage
static void Init_Hann_Window(void)
{
    for (int n = 0; n < FFT_SIZE; n++)
    {
        window[n] = 0.5f * (1.0f - arm_cos_f32(2.0f * PI * n / (FFT_SIZE - 1)));
    }
}

// Spectral mask defaults to all-pass (1.0). Uncomment filter sections below to enable filtering.
static void Init_Spectral_Mask(void)
{

    for (int k = 0; k <= FFT_SIZE / 2; k++)
    {
        spectral_mask[k] = 1.0f;
    }

    // Low-pass filter: Remove high frequencies above 4kHz
    /*
    float cutoff_freq = 4000.0f;
    int cutoff_bin = (int)(cutoff_freq * FFT_SIZE / SAMPLE_RATE);
    for (int k = cutoff_bin; k <= FFT_SIZE / 2; k++)
    {
        spectral_mask[k] = 0.0f;  // Zero out high frequencies
    }
    */

    // Notch filter: Remove specific frequency (e.g., 1kHz hum)
    /*
    float notch_freq = 1000.0f;
    int notch_bin = (int)(notch_freq * FFT_SIZE / SAMPLE_RATE);
    int notch_width = 2;  // Bins to zero on each side
    for (int k = notch_bin - notch_width; k <= notch_bin + notch_width; k++)
    {
        if (k >= 0 && k <= FFT_SIZE / 2)
        {
            spectral_mask[k] = 0.0f;
        }
    }
    */

    // Band-pass filter: Keep only voice frequencies (300Hz-3400Hz)
    /*
    float low_freq = 300.0f;
    float high_freq = 3400.0f;
    int low_bin = (int)(low_freq * FFT_SIZE / SAMPLE_RATE);
    int high_bin = (int)(high_freq * FFT_SIZE / SAMPLE_RATE);
    for (int k = 0; k <= FFT_SIZE / 2; k++)
    {
        if (k < low_bin || k > high_bin)
        {
            spectral_mask[k] = 0.0f;  // Zero out frequencies outside voice band
        }
    }
    */

    // High-pass filter: Remove low-frequency rumble below 100Hz
    /*
    float highpass_freq = 100.0f;
    int highpass_bin = (int)(highpass_freq * FFT_SIZE / SAMPLE_RATE);
    for (int k = 0; k <= highpass_bin; k++)
    {
        spectral_mask[k] = 0.0f;  // Zero out low frequencies
    }
    */
}

// FFT output format: [Real0, Real_N/2, Real1, Imag1, ...], DC and Nyquist bins are real-only
static void Apply_Spectral_Mask(float32_t *spectrum)
{
    spectrum[0] *= spectral_mask[0];
    spectrum[1] *= spectral_mask[FFT_SIZE / 2];

    for (int k = 1; k < FFT_SIZE / 2; k++)
    {
        int idx = k * 2;
        spectrum[idx] *= spectral_mask[k];
        spectrum[idx + 1] *= spectral_mask[k];
    }
}

// Window -> FFT -> spectral mask -> IFFT -> window again for COLA compliance
static void Process_FFT_Block(float32_t *input, float32_t *output)
{
  static float32_t fft_buffer[FFT_SIZE];
  arm_mult_f32(input, window, fft_buffer, FFT_SIZE);
  arm_rfft_fast_f32(&fft_instance, fft_buffer, output, 0);
  Apply_Spectral_Mask(output);
  arm_rfft_fast_f32(&fft_instance, output, fft_buffer, 1);

  arm_mult_f32(fft_buffer, window, output, FFT_SIZE);
}


/**
  * @brief  Initialize audio processing module
  * @param  None
  */
uint8_t Audio_Init(void)
{
    // Initialize FFT instance
    arm_status status = arm_rfft_fast_init_f32(&fft_instance, FFT_SIZE);
    if (status != ARM_MATH_SUCCESS)
    {
        return AUDIO_ERROR;
    }

    // Initialize window and spectral mask
    Init_Hann_Window();
    Init_Spectral_Mask();

    memset(overlap_buffer, 0, sizeof(overlap_buffer));
    memset(input_history, 0, sizeof(input_history));

    return AUDIO_OK;
}

/**
  * @brief  Process one block of audio samples
  * @param  adc_samples: Pointer to ADC input samples (uint16_t, HOP_SIZE length)
  * @param  i2s_samples: Pointer to I2S output buffer (int16_t, HOP_SIZE*2 stereo)
  */
void Audio_ProcessBlock(uint16_t *adc_samples, int16_t *i2s_samples)
{
    float32_t temp_buffer[HOP_SIZE];

    memcpy(fft_input, input_history, HOP_SIZE * sizeof(float32_t));

    for (int i = 0; i < HOP_SIZE; i++)
    {
        float32_t raw_val = (float32_t)adc_samples[i];
        dc_offset = (1.0f - DC_ALPHA) * dc_offset + DC_ALPHA * raw_val;
        fft_input[HOP_SIZE + i] = (raw_val - dc_offset) * (1.0f / 2048.0f);
    }

    memcpy(input_history, &fft_input[HOP_SIZE], HOP_SIZE * sizeof(float32_t));
    Process_FFT_Block(fft_input, fft_output);

    float32_t next_overlap[HOP_SIZE];
    memcpy(next_overlap, &fft_output[HOP_SIZE], HOP_SIZE * sizeof(float32_t));

    arm_add_f32(fft_output, overlap_buffer, temp_buffer, HOP_SIZE);
    arm_scale_f32(temp_buffer, AUDIO_GAIN, temp_buffer, HOP_SIZE);

    for (int i = 0; i < HOP_SIZE; i++)
    {
        if (temp_buffer[i] > 1.0f) temp_buffer[i] = 1.0f;
        if (temp_buffer[i] < -1.0f) temp_buffer[i] = -1.0f;
    }

    int16_t temp_q15[HOP_SIZE];
    arm_float_to_q15(temp_buffer, temp_q15, HOP_SIZE);

    for (int i = 0; i < HOP_SIZE; i++)
    {
        i2s_samples[i * 2] = temp_q15[i];
        i2s_samples[i * 2 + 1] = temp_q15[i];
    }

    memcpy(overlap_buffer, next_overlap, HOP_SIZE * sizeof(float32_t));
}

/**
  * @brief  Get current DC offset estimate
  * @param  None
  * @retval Current DC offset value
  */
float32_t Audio_GetDCOffset(void)
{
    return dc_offset;
}
