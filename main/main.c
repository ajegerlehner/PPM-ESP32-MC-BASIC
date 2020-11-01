/**
 * Name:            main.c
 * Description:     Source for entry point to application for the  
 *                  PPM IoT machine counter board using ESP-32 and 
 *                  ST32F04 microcontrollers. 
 *                  Collects sensor data and sends it to Azure IoT event hub
 * Initial Date:    2020-06-03
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_eth.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"
#include "tcpip_adapter.h"

#include "mc_common.h"
#include "mc_data.h"
#include "mc_eth.h"
#include "mc_sensor_data.h"
#include "mc_azure_hub_client.h"
#include "mc_oled_display.h"
#include "mc_iotcentral.h"
#include "mc_gpio.h"
#include "mc_device.h"
#include "mc_mcp23071.h"
#include "mc_stm32f04.h"
#include "mc_ds3231.h"

#define TAG "MAIN"

#define HEARTBEAT_TIME_INTERVAL 600

char *iotDeviceId;
DeviceConfig deviceConfig;  
SamplingMessage samplingMsg;
DeviceStatus deviceStatus;
char jsonSamplingMsg[AZURE_MSG_SIZE_MAX];
char jsonStatusMsg[AZURE_MSG_SIZE_MAX];

QueueHandle_t statusMsgQueue;

SemaphoreHandle_t startUpMutex;

IOTHUB_DEVICE_CLIENT_LL_HANDLE azure_client;

// initialize the board and wifi connection
void board_init()
{    
    // initialize nvs flash storage
    ESP_ERROR_CHECK(nvs_init());    
    
    // init esp event loop and tcpip adapter
    ESP_ERROR_CHECK(prepare_network());
    
    // initialize ethernet PHY connection
    ESP_ERROR_CHECK(phy_init());    
    vTaskDelay(ONE_SECOND_DELAY * 5);    
    
    // initialize wifi connection
    ESP_ERROR_CHECK(wifi_init());

    // read iotDeviceId from nvs flash
    ESP_ERROR_CHECK(init_iotDeviceId());

    // initialize I2C communication
    i2c_bus_init();
    
    // initialize and setup OLED display
    oled_init(); 

    // setup GPIO ports
    gpio_init();    
    
    // initialize and setup MCP230XX port expanders
    mcp230xx_init();

    // initialize sensor data structures
    init_sensor_hardware();

    // initialize communication with STM32 microcontroller
    stm32_uart_install();

    // create queue for status messages
    statusMsgQueue = xQueueCreate(10, sizeof(DeviceStatus));
}

void enqueueStatusMsg(DeviceStatus *statusMsg, SamplingMessage *samplingMsg, char *statusTxt)
{
    create_status_msg(statusMsg, samplingMsg, statusTxt);   

    xQueueSend(statusMsgQueue, (void*)statusMsg, (TickType_t) 5);
}

void azure_status_task(void *pvParameter)
{
    int jsonLen;    
    DeviceStatus devStatus;

    // wait for azure connection to be initialized
    while (azure_client == NULL);

    while (1)
    {
        while (statusMsgQueue != 0)
        {
            if (xQueueReceive(statusMsgQueue, (void*) &devStatus, (TickType_t) 5))
            {
                jsonLen = serializeStatusgMessage(&devStatus, jsonStatusMsg);
                ESP_LOGI(TAG, "Azure Status Msg: %s", jsonStatusMsg);
                mcp230xx_signal_pin(MCP23017_AZURE_PIN, MCP23008_AZURE_PIN, SIGNAL_PIN_DELAY);

                //Send status message to Azure IoT Hub
                int sendresult = send_azure_msgs(jsonStatusMsg, azure_client);                
                if (sendresult == 0)
                {
                    ESP_LOGI(TAG, "Status message no. %ld queued to be sent to Azure, %d bytes", devStatus.msgNum, jsonLen);                        
                }
                else
                {
                    ESP_LOGE(TAG, "Status message not queued for Azure");
                }
            }

        }
        
        vTaskDelay(ONE_SECOND_DELAY * deviceConfig.deviceToCloudInterval);   
    }

    vTaskDelete(NULL);
}

void enqueueHeartbeatStatusMsg(void *pvParameter)
{
    // wait for startup completing
    while (xSemaphoreTake(startUpMutex, (TickType_t) 20) != 1);
    xSemaphoreGive(startUpMutex);

    while (1)
    {
            enqueueStatusMsg(&deviceStatus, &samplingMsg, "Heartbeat.");

            vTaskDelay(ONE_SECOND_DELAY * HEARTBEAT_TIME_INTERVAL);
    }

    vTaskDelete(NULL);
}

// task to send sensor data to Azure
void azure_task(void *pvParameter)
{
    int jsonLen;    
    time_t now;

    while (xSemaphoreTake(startUpMutex, (TickType_t) 10) != 1);

    ESP_LOGI(TAG, "Device to cloud interval is %d seconds.", deviceConfig.deviceToCloudInterval);
    
    // setup watchdog timer
    ESP_LOGI(TAG, "WDT initialized with %d seconds.", deviceConfig.watchdogTimeFence);
    esp_task_wdt_init(deviceConfig.watchdogTimeFence, true);
    esp_task_wdt_add(NULL);
    mcp23017_set_pin(MCP23017_WDE_PIN, 1);

    // boot time stamp
    time(&now);
    deviceStatus.lastBootTime = now;

    // initialize sensor data struct
    init_sensor_data(&samplingMsg, CONFIG_PPM_DEVICE_ID, "data");

    // initialize status message struct
    init_devicve_status(&deviceStatus, CONFIG_PPM_DEVICE_ID, "status");
    
    // Create status message to send to Azure IoT
    enqueueStatusMsg(&deviceStatus, &samplingMsg, "Starting up.");

    xSemaphoreGive(startUpMutex);

    while (1)
    {
        // get sesnor data readings in sampling message        
        read_sensor_data(&samplingMsg, deviceConfig);
        
        // Create message to send to Azure IoT
        jsonLen = serializeSamplingMessage(&samplingMsg, jsonSamplingMsg);
        ESP_LOGI(TAG, "Azure Msg: %s", jsonSamplingMsg);

        //Send message to Azure IoT Hub
        mcp230xx_signal_pin(MCP23017_AZURE_PIN, MCP23008_AZURE_PIN, SIGNAL_PIN_DELAY);

        int sendresult = send_azure_msgs(jsonSamplingMsg, azure_client);                
        if (sendresult == 0)
        {
            ESP_LOGI(TAG, "Message no. %ld queued to be sent to Azure, %d bytes", samplingMsg.msgNum, jsonLen);                        
        }
        else
        {
            ESP_LOGE(TAG, "Message Not Queued for Azure");
        }

        //Process Any Messages
        process_azure_msgs(azure_client);
        vTaskDelay(ONE_SECOND_DELAY *2);

        // reset watchdog timer
        mcp230xx_signal_pin(MCP23017_WDT_PIN, MCP23008_WDT_PIN, SIGNAL_PIN_DELAY);
        esp_task_wdt_reset();

        //Delay for deviceToCloudInterval seconds
        vTaskDelay(ONE_SECOND_DELAY * deviceConfig.deviceToCloudInterval);     
    }
    
    //Clean up Azure IoT Connection
    destroy_iothub_client(azure_client);

    vTaskDelete(NULL);
}


// task to display sensor data on build in OLED display
void show_sensor_task(void *pvParameter)
{
    // wait for statup procedure to complete
    while (xSemaphoreTake(startUpMutex, (TickType_t) 10) != 1);

    // free mutex 
    xSemaphoreGive(startUpMutex);

    while(1)
    {
        oled_write_data(&samplingMsg, 0);
        
        vTaskDelay(ONE_SECOND_DELAY *10);
    }
    
    vTaskDelete(NULL);  
}

// app_main: main entry point
void app_main()
{
    int taskResult;    
    esp_err_t ds3231_status;
    
    board_init();
    vTaskDelay(ONE_SECOND_DELAY);
    ESP_LOGI(TAG, "ESP32 board initialized. Device id is: %s", iotDeviceId);

    startUpMutex = xSemaphoreCreateMutex();

    // initialize DS3231 realtime clock
    ds3231_status = ds3231_init();    

    azure_client = init_iothub_client(iotDeviceId);
    if (azure_client == NULL)
    {
        ESP_LOGE(TAG, "APP INFO: No Azure IoT Hub Handle was obtained");                
    }

    // ds3231 has been reset, initialize time from internet time
    if (ds3231_status != ESP_OK)
    {
        ds3231_init_time();
    }

    // setup Azure Iot ebent hub device twin
    setupDeviceTwin(azure_client);

    // setup device callback methods
    setupDeviceCallbackMethods(azure_client);

    
    ESP_LOGI(TAG, "Waiting for device config..."); 
    while (strlen(deviceConfig.iotDeviceId) == 0 )
    {
        ESP_LOGI(TAG, "Waiting for device config..."); 
        vTaskDelay(ONE_SECOND_DELAY);
    }
    ESP_LOGI(TAG, "iotDeviceId is %s", deviceConfig.iotDeviceId); 
    
    //High Priority Task for sending telementry data to Azure
    taskResult = xTaskCreate(&azure_task, "azure_task", 1024 * 6, NULL, 5, NULL); 
    if (taskResult != pdPASS)
    {   
        ESP_LOGE(TAG, "Create azure task failed"); 
    }

    // Low priority task to send status messages to Azure IoT hub
    taskResult = xTaskCreate(&azure_status_task, "enqueueStatusMsg_task", 1024 * 6, NULL, 4, NULL); 
    if (taskResult != pdPASS)
    {   
         ESP_LOGE(TAG, "Create EnqueueStatusMsg task failed"); 
    }

    // Low priority task to send heartbeat status msg to Azure IoT hub
    taskResult = xTaskCreate(&enqueueHeartbeatStatusMsg, "enqueueHeartbeatStatusMsg_task", 1024 * 6, NULL, 3, NULL); 
    if (taskResult != pdPASS)
    {   
         ESP_LOGE(TAG, "Create EnqueueHeartBeatStatusMsg task failed"); 
    }

    //Low Medium Priority Task for displaying sensor data on OLED display
    taskResult = xTaskCreate(&show_sensor_task, "show_sensor_task", 1024 * 4 , NULL, 3, NULL); 
    if (taskResult != pdPASS)
    {   
        ESP_LOGE(TAG, "Create show sensor task failed"); 
    }
}

