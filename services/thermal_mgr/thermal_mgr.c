#include "thermal_mgr.h"
#include "errors.h"
#include "lm75bd.h"
#include "console.h"
#include "logging.h"

#include <FreeRTOS.h>
#include <os_task.h>
#include <os_queue.h>

#include <string.h>

#define THERMAL_MGR_STACK_SIZE 256U

static TaskHandle_t thermalMgrTaskHandle;
static StaticTask_t thermalMgrTaskBuffer;
static StackType_t thermalMgrTaskStack[THERMAL_MGR_STACK_SIZE];

#define THERMAL_MGR_QUEUE_LENGTH 10U
#define THERMAL_MGR_QUEUE_ITEM_SIZE sizeof(thermal_mgr_event_t)

static QueueHandle_t thermalMgrQueueHandle;
static StaticQueue_t thermalMgrQueueBuffer;
static uint8_t thermalMgrQueueStorageArea[THERMAL_MGR_QUEUE_LENGTH * THERMAL_MGR_QUEUE_ITEM_SIZE];

static void thermalMgr(void *pvParameters);

void initThermalSystemManager(lm75bd_config_t *config) {
  memset(&thermalMgrTaskBuffer, 0, sizeof(thermalMgrTaskBuffer));
  memset(thermalMgrTaskStack, 0, sizeof(thermalMgrTaskStack));
  
  thermalMgrTaskHandle = xTaskCreateStatic(
    thermalMgr, "thermalMgr", THERMAL_MGR_STACK_SIZE,
    config, 1, thermalMgrTaskStack, &thermalMgrTaskBuffer);

  memset(&thermalMgrQueueBuffer, 0, sizeof(thermalMgrQueueBuffer));
  memset(thermalMgrQueueStorageArea, 0, sizeof(thermalMgrQueueStorageArea));

  thermalMgrQueueHandle = xQueueCreateStatic(
    THERMAL_MGR_QUEUE_LENGTH, THERMAL_MGR_QUEUE_ITEM_SIZE,
    thermalMgrQueueStorageArea, &thermalMgrQueueBuffer);

}

error_code_t thermalMgrSendEvent(thermal_mgr_event_t *event) {
  if (event == NULL) {
    return ERR_CODE_INVALID_ARG;
  } else if (thermalMgrQueueHandle == NULL) {
    return ERR_CODE_INVALID_STATE;
  }

  if(xQueueSend(thermalMgrQueueHandle, (void *)event, 0) == pdPASS) {
    return ERR_CODE_SUCCESS;
  } else {
    return ERR_CODE_QUEUE_FULL;
  }
}

// triggered on an interrupt sent
// interrupt means either we've exceeded the overtemp threshold or gone back down below it (hysteresis)
void osHandlerLM75BD(void) {
  thermal_mgr_event_t interruptEvent;
  interruptEvent.type = THERMAL_MGR_EVENT_HANDLE_INTERRUPT;

  thermalMgrSendEvent(&interruptEvent); // Register read is taken care of as temperature is checked
}

static void thermalMgr(void *pvParameters) {
  thermal_mgr_event_t queueMsg;
  
  while (1) {
    if(xQueueReceive(thermalMgrQueueHandle, &queueMsg, 0) == pdPASS) {
      float temp;
      
      error_code_t result = readTempLM75BD(LM75BD_OBC_I2C_ADDR, &temp);
      result = ERR_CODE_INVALID_STATE;
      
      if (result != ERR_CODE_SUCCESS) {
        LOG_ERROR_CODE(result);
      }

      if(queueMsg.type == THERMAL_MGR_EVENT_MEASURE_TEMP_CMD) {
        addTemperatureTelemetry(temp);
      } else if (queueMsg.type == THERMAL_MGR_EVENT_HANDLE_INTERRUPT) {
        if(temp > OS_TH_TEMP) {
          overTemperatureDetected();
        } else {
          safeOperatingConditions();
        }
      } else {
        LOG_ERROR_CODE(ERR_CODE_INVALID_QUEUE_MSG);
      }
    }
  }
}

void addTemperatureTelemetry(float tempC) {
  printConsole("Temperature telemetry: %f deg C\n", tempC);
}

void overTemperatureDetected(void) {
  printConsole("Over temperature detected!\n");
}

void safeOperatingConditions(void) { 
  printConsole("Returned to safe operating conditions!\n");
}
