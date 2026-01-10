/**
  ******************************************************************************
  * @file    audio_processing.h
  * @brief   Header file for real-time audio DSP processing
  * @author  Berkan Tali
  * @date    2025
  ******************************************************************************
  * @attention
  *
  * This module implements overlap-add FFT-based audio processing:
  * - FFT size: 512 samples
  * - Hop size: 256 samples (50% overlap)
  * - Sample rate: 48kHz
  * - Hann windowing to reduce spectral leakage
  * - Configurable spectral filtering
  *
  ******************************************************************************
  */

#ifndef AUDIO_PROCESSING_H
#define AUDIO_PROCESSING_H

#include "stm32f4xx_hal.h"
#include "arm_math.h"

// DSP Configuration
#define FFT_SIZE            512    // FFT length in samples
#define HOP_SIZE            256    // Hop size (50% overlap)
#define SAMPLE_RATE         48000  // ADC/DAC sample rate in Hz

// Audio Processing Parameters
#define AUDIO_GAIN          0.5f        // Output gain multiplier
#define NOISE_GATE_THRESH   0.02f       // Noise gate threshold
#define DC_ALPHA            0.001f       // DC offset filter coefficient (lower = slower tracking)

// Function return codes
#define AUDIO_OK      0
#define AUDIO_ERROR   1


/**
  * @brief  Initialize audio processing module
  * @param  None
  * @retval AUDIO_OK on success, AUDIO_ERROR on failure
  * @note   Must be called before starting audio processing
  *         Initializes FFT, window function, and spectral mask
  */
uint8_t Audio_Init(void);

/**
  * @brief  Process one block of audio samples
  * @param  adc_samples: Pointer to ADC input samples (uint16_t, HOP_SIZE length)
  * @param  i2s_samples: Pointer to I2S output buffer (int16_t, HOP_SIZE*2 stereo)
  * @retval None
  * @note   Performs: ADC conversion -> FFT -> Spectral filtering -> IFFT -> Output
  */
void Audio_ProcessBlock(uint16_t *adc_samples, int16_t *i2s_samples);

/**
  * @brief  Get current DC offset estimate
  * @param  None
  * @retval Current DC offset value
  * @note   Useful for debugging ADC bias
  */
float32_t Audio_GetDCOffset(void);

#endif //  AUDIO_PROCESSING_H