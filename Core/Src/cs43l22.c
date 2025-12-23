/**
  ******************************************************************************
  * @file    cs43l22.c
  * @brief   CS43L22 audio codec driver implementation
  * @author  Berkan Tali
  * @date    2025
  ******************************************************************************
  */

#include "cs43l22.h"
#include "main.h"

/* External I2C handle (defined in main.c) */
extern I2C_HandleTypeDef hi2c1;

/**
  * @brief  Hardware reset of CS43L22 codec
  * @param  None
  * @retval None
  */
void CS43L22_Reset(void)
{
    // Assert reset (active low) 
    HAL_GPIO_WritePin(CS43l22_RST_GPIO_Port, CS43l22_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);  // Hold reset for 5ms
    
    // De-assert reset 
    HAL_GPIO_WritePin(CS43l22_RST_GPIO_Port, CS43l22_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(5);  // Wait for codec to stabilize
}

/**
  * @brief  Write a value to CS43L22 register
  * @param  reg: Register address
  * @param  value: Data to write
  * @retval CS43L22_OK on success, CS43L22_ERROR on I2C failure
  */
uint8_t CS43L22_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    
    /* Transmit register address and data */
    if (HAL_I2C_Master_Transmit(&hi2c1, CS43L22_ADDR, data, 2, 100) != HAL_OK)
    {
        return CS43L22_ERROR;
    }
    
    return CS43L22_OK;
}

/**
  * @brief  Initialize CS43L22 codec
  * @param  None
  * @retval CS43L22_OK on success, CS43L22_ERROR on failure
  * @note   Based on CS43L22 datasheet initialization sequence
  */
uint8_t CS43L22_Init(void)
{
    uint8_t reg_value;
    HAL_StatusTypeDef status;
    if (CS43L22_WriteReg(CS43L22_REG_POWER_CTL1, 0x01) != CS43L22_OK)
        return CS43L22_ERROR;

    // Write 0x99 to register 0x00 (undocumented but required)
    if (CS43L22_WriteReg(0x00, 0x99) != CS43L22_OK)
        return CS43L22_ERROR;
    
    // Write 0x80 to register 0x47 (undocumented but required)
    if (CS43L22_WriteReg(0x47, 0x80) != CS43L22_OK)
        return CS43L22_ERROR;
    
    // Read-modify-write register 0x32 - set bit 7
    status = HAL_I2C_Master_Transmit(&hi2c1, CS43L22_ADDR, (uint8_t[]){0x32}, 1, 100);
    if (status != HAL_OK)
        return CS43L22_ERROR;
    
    status = HAL_I2C_Master_Receive(&hi2c1, CS43L22_ADDR, &reg_value, 1, 100);
    if (status != HAL_OK)
        return CS43L22_ERROR;
    
    if (CS43L22_WriteReg(0x32, reg_value | 0x80) != CS43L22_OK)
        return CS43L22_ERROR;
    
    // Read-modify-write register 0x32 - clear bit 7
    status = HAL_I2C_Master_Transmit(&hi2c1, CS43L22_ADDR, (uint8_t[]){0x32}, 1, 100);
    if (status != HAL_OK)
        return CS43L22_ERROR;
    
    status = HAL_I2C_Master_Receive(&hi2c1, CS43L22_ADDR, &reg_value, 1, 100);
    if (status != HAL_OK)
        return CS43L22_ERROR;
    
    if (CS43L22_WriteReg(0x32, reg_value & 0x7F) != CS43L22_OK)
        return CS43L22_ERROR;
    
    // Write 0x00 to register 0x00 (exit init mode)
    if (CS43L22_WriteReg(0x00, 0x00) != CS43L22_OK)
        return CS43L22_ERROR;

    // 0xAF = Headphone A/B powered, Speaker A/B powered
    if (CS43L22_WriteReg(CS43L22_REG_POWER_CTL2, 0xAF) != CS43L22_OK)
        return CS43L22_ERROR;

    // 0x81 = Auto-detect sample rate, MCLK/2
    if (CS43L22_WriteReg(CS43L22_REG_CLOCKING_CTL, 0x81) != CS43L22_OK)
        return CS43L22_ERROR;

    // 0x04 = I2S format, 16-bit data
    if (CS43L22_WriteReg(CS43L22_REG_INTERFACE_CTL1, 0x04) != CS43L22_OK)
        return CS43L22_ERROR;
    
    // 0x04 = Disable soft ramp and zero cross
    if (CS43L22_WriteReg(CS43L22_REG_MISC_CTL, 0x04) != CS43L22_OK)
        return CS43L22_ERROR;

    // 0x00 = Normal operation
    if (CS43L22_WriteReg(CS43L22_REG_PLAYBACK_CTL1, 0x00) != CS43L22_OK)
        return CS43L22_ERROR;
    
    // Set PCM volume to 0dB (no attenuation)
    if (CS43L22_WriteReg(CS43L22_REG_PCMA_VOL, 0x00) != CS43L22_OK)
        return CS43L22_ERROR;
    if (CS43L22_WriteReg(CS43L22_REG_PCMB_VOL, 0x00) != CS43L22_OK)
        return CS43L22_ERROR;

    // 0x0F = Bass/Treble at 0dB
    if (CS43L22_WriteReg(CS43L22_REG_TONE_CTL, 0x0F) != CS43L22_OK)
        return CS43L22_ERROR;
    
    // Set master volume to maximum (255 = 0dB) 
    if (CS43L22_WriteReg(CS43L22_REG_MASTER_A_VOL, 255) != CS43L22_OK)
        return CS43L22_ERROR;
    if (CS43L22_WriteReg(CS43L22_REG_MASTER_B_VOL, 255) != CS43L22_OK)
        return CS43L22_ERROR;

    if (CS43L22_WriteReg(CS43L22_REG_HP_A_VOL, 255) != CS43L22_OK)
        return CS43L22_ERROR;
    if (CS43L22_WriteReg(CS43L22_REG_HP_B_VOL, 255) != CS43L22_OK)
        return CS43L22_ERROR;
    /* 0x9E = Power up, enable outputs */
    if (CS43L22_WriteReg(CS43L22_REG_POWER_CTL1, 0x9E) != CS43L22_OK)
        return CS43L22_ERROR;
    
    return CS43L22_OK;
}