/**
  ******************************************************************************
  * @file    cs43l22.h
  * @brief   Header file for CS43L22 audio codec driver
  ******************************************************************************
  * @attention
  *
  * CS43L22 is a stereo DAC with built-in headphone amplifier
  * Communication: I2C for control, I2S for audio data
  * This driver handles initialization and basic control
  *
  ******************************************************************************
  */

#ifndef CS43L22_H
#define CS43L22_H

#include "stm32f4xx_hal.h"

// CS43L22 I2C Address (7-bit address shifted left by hardware)
#define CS43L22_ADDR        0x94

// CS43L22 Register Map
#define CS43L22_REG_ID              0x01  // Chip ID register
#define CS43L22_REG_POWER_CTL1      0x02  // Power control 1
#define CS43L22_REG_POWER_CTL2      0x04  // Power control 2
#define CS43L22_REG_CLOCKING_CTL    0x05  // Clocking control
#define CS43L22_REG_INTERFACE_CTL1  0x06  // Interface control 1
#define CS43L22_REG_PLAYBACK_CTL1   0x0D  // Playback control
#define CS43L22_REG_MISC_CTL        0x0E  // Miscellaneous controls
#define CS43L22_REG_TONE_CTL        0x1F  // Tone control
#define CS43L22_REG_PCMA_VOL        0x1A  // PCM channel A volume
#define CS43L22_REG_PCMB_VOL        0x1B  // PCM channel B volume
#define CS43L22_REG_MASTER_A_VOL    0x20  // Master A volume
#define CS43L22_REG_MASTER_B_VOL    0x21  // Master B volume
#define CS43L22_REG_HP_A_VOL        0x22  // Headphone A volume
#define CS43L22_REG_HP_B_VOL        0x23  // Headphone B volume

// Function return codes
#define CS43L22_OK      0
#define CS43L22_ERROR   1

/**
  * @brief  Hardware reset of CS43L22 codec
  * @param  None
  * @retval None
  * @note   Requires GPIO pin configured for RESET control
  */
void CS43L22_Reset(void);

/**
  * @brief  Write a value to CS43L22 register
  * @param  reg: Register address
  * @param  value: Data to write
  * @retval CS43L22_OK on success, CS43L22_ERROR on I2C failure
  */
uint8_t CS43L22_WriteReg(uint8_t reg, uint8_t value);

/**
  * @brief  Initialize CS43L22 codec
  * @param  None
  * @retval CS43L22_OK on success, CS43L22_ERROR on failure
  * @note   Configures codec for I2S input, 46.875kHz, stereo output
  */
uint8_t CS43L22_Init(void);

#endif /* CS43L22_H */
