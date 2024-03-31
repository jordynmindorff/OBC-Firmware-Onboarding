#include "lm75bd.h"
#include "i2c_io.h"
#include "errors.h"
#include "logging.h"

#include <stdint.h>
#include <string.h>
#include <math.h>

/* LM75BD Registers (p.8) */
#define LM75BD_REG_CONF 0x01U  /* Configuration Register (R/W) */

error_code_t lm75bdInit(lm75bd_config_t *config) {
  error_code_t errCode;

  if (config == NULL) return ERR_CODE_INVALID_ARG;

  RETURN_IF_ERROR_CODE(writeConfigLM75BD(config->devAddr, config->osFaultQueueSize, config->osPolarity,
                                         config->osOperationMode, config->devOperationMode));

  // Assume that the overtemperature and hysteresis thresholds are already set
  // Hysteresis: 75 degrees Celsius
  // Overtemperature: 80 degrees Celsius

  return ERR_CODE_SUCCESS;
}

error_code_t readTempLM75BD(uint8_t devAddr, float *temp) {
  /* Implement this driver function */
  error_code_t errCode;
  uint8_t buff[2] = {0x0};
  uint16_t signMask = 0x0400; // The 11th bit from right (aka bit 10) in our raw int will represent the sign
  uint16_t elevenBitMask = 0x07ff; // To do bitwise operations to basically treat a uint16_t like it only has 11 bits

  RETURN_IF_ERROR_CODE(i2cSendTo(devAddr, buff, 1)); // Set pointer register for temp read
  RETURN_IF_ERROR_CODE(i2cReceiveFrom(devAddr, buff, 2)); // Recieve 2 temperature bytes

  // Collect 11 meaningful bits into a 16-bit integer
  uint16_t raw = buff[0];
  raw <<= 3; 
  raw |= buff[1] >> 5;

  // Assign a value to the provided float, dependent on sign
  if (raw & signMask) {
    // Two's complement on raw -> positive representation
    raw = raw ^ elevenBitMask;
    raw += 1;

    raw &= elevenBitMask; // Ignore any potential overflow to the 12th bit from right

    *temp = -(raw) * 0.125f;
  } else {
    *temp = (raw) * 0.125f;
  }
  
  return ERR_CODE_SUCCESS;
}

#define CONF_WRITE_BUFF_SIZE 2U
error_code_t writeConfigLM75BD(uint8_t devAddr, uint8_t osFaultQueueSize, uint8_t osPolarity,
                                   uint8_t osOperationMode, uint8_t devOperationMode) {
  error_code_t errCode;

  // Stores the register address and data to be written
  // 0: Register address
  // 1: Data
  uint8_t buff[CONF_WRITE_BUFF_SIZE] = {0};

  buff[0] = LM75BD_REG_CONF;

  uint8_t osFaltQueueRegData = 0;
  switch (osFaultQueueSize) {
    case 1:
      osFaltQueueRegData = 0;
      break;
    case 2:
      osFaltQueueRegData = 1;
      break;
    case 4:
      osFaltQueueRegData = 2;
      break;
    case 6:
      osFaltQueueRegData = 3;
      break;
    default:
      return ERR_CODE_INVALID_ARG;
  }

  buff[1] |= (osFaltQueueRegData << 3);
  buff[1] |= (osPolarity << 2);
  buff[1] |= (osOperationMode << 1);
  buff[1] |= devOperationMode;

  errCode = i2cSendTo(LM75BD_OBC_I2C_ADDR, buff, CONF_WRITE_BUFF_SIZE);
  if (errCode != ERR_CODE_SUCCESS) return errCode;

  return ERR_CODE_SUCCESS;
}
