#pragma once

#include "lm75bd.h"
#include "errors.h"

typedef enum {
  THERMAL_MGR_EVENT_MEASURE_TEMP_CMD,
  THERMAL_MGR_EVENT_HANDLE_INTERRUPT
} thermal_mgr_event_type_t;

typedef struct {
  thermal_mgr_event_type_t type;
} thermal_mgr_event_t;

#define OS_TH_TEMP 80
#define OS_HYS_TEMP 75

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the thermal system manager
 *
 * @param config Configuration struct for LM75BD
 */
void initThermalSystemManager(lm75bd_config_t *config);

/**
 * @brief Send an event to the thermal manager task's queue
 *
 * @param event Thermal manager event to add to queue
 * @return ERR_CODE_SUCCESS if successful, error code otherwise
 */
error_code_t thermalMgrSendEvent(thermal_mgr_event_t *event);

/**
 * @brief Log temperature telemetry to console
 *
 * @param tempC Temperature recieved in Celsius
 */
void addTemperatureTelemetry(float tempC);

/**
 * @brief Log to console when the overtemperature threshold is reached
*/
void overTemperatureDetected(void);

/**
 * @brief Log to console when the temperature returns to safe levels after an overtemperatue event
*/
void safeOperatingConditions(void);

#ifdef __cplusplus
}
#endif
