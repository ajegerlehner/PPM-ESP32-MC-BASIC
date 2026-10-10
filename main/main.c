/**
 * Name:            main.c
 * Description:     Source for entry point to application for the  
 *                  PPM IoT machine counter board using ESP-32 
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
#include "esp_netif.h"

#include "mc_common.h"
#include "mc_data.h"
#include "mc_eth.h"
#include "mc_sensor_data.h"
#include "mc_azure_hub_client.h"
#include "mc_oled_display.h"
#include "mc_iotcentral.h"
#include "mc_gpio.h"
#include "mc_device.h"
#include "mc_io_expander_hal.h"

#define TAG "MAIN"

#define OLED_SCREENS    10
#define MAX_IP_RETRYS   20

#define BUTTON_HOME     1
#define BUTTON_FWD      2
#define BUTTON_BCK      4

char *iotDeviceId;          // global iot device id
char *deviceIPAddress;      // global ip address
int  internetConnected = 0; // global flag for ip address received
uint8_t wifiConnected = 0;  // global flag for connection type

DeviceConfig deviceConfig;  
SamplingMessage samplingMsg;
DeviceStatus deviceStatus;
DeviceHWStatus deviceHWStatus;

static char jsonSamplingMsg[AZURE_MSG_SIZE_MAX];
static char jsonStatusMsg[AZURE_MSG_SIZE_MAX];

QueueHandle_t statusMsgQueue;
QueueHandle_t telemetryQueue;

SemaphoreHandle_t startUpMutex;
SemaphoreHandle_t oledDisplayMutex;

mc_iothub_handle_t azure_client;

uint8_t currentOLEDScreen;

void log_startup_device_status()
{
    ESP_LOGI(TAG, "MCP23008 io expander display present: %d", deviceHWStatus.mcp23008d_present);
    ESP_LOGI(TAG, "MCP23017 io expander display present: %d", deviceHWStatus.mcp23017d_present);
    ESP_LOGI(TAG, "PCA9554A io expander display present: %d", deviceHWStatus.pca9554a_present);
    ESP_LOGI(TAG, "SI7021 sensor present: %d", deviceHWStatus.si7021_present);
    ESP_LOGI(TAG, "VL6180X sensor present: %d", deviceHWStatus.vl6180x_present);
}

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

    // setup GPIO ports
    gpio_init();    

    // check if NVS RESET pin is pressed - enter config mode
    ESP_LOGI(TAG, "Checking NVS RESET pin.");
    if (check_nvs_reset_pin())
    {
        ESP_LOGI(TAG, "NVS RESET pin pressed - entering config mode.");
        enter_config_mode();
    }
    else
    {
        ESP_LOGI(TAG, "NVS RESET pin is low - continue normal operation.");
    }

    // read iotDeviceId from nvs flash
    ESP_ERROR_CHECK(init_iotDeviceId());

    // initialize I2C communication
    i2c_bus_init();
    
    // initialize and setup OLED display
    oled_init(); 
    
    // initialize port expanders
    io_expander_init();

    // led test
    io_expander_led_test();

    // initialize sensor data structures
    init_sensor_hardware();

    // signal end of boot process
    io_expander_signal_boot_end();

    // create queue for status messages
    statusMsgQueue = xQueueCreate(10, sizeof(DeviceStatus));
    //telemetryQueue = xQueueCreate(5000, 600);

    log_startup_device_status();
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
                io_expander_signal_pin(SIGNAL_PIN_AZURE);

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

        // reset status queue
        xQueueReset(statusMsgQueue);
        
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

            vTaskDelay(ONE_SECOND_DELAY * CONFIG_PPM_IOT_HEARTBEAT_INTERVALL);
    }

    vTaskDelete(NULL);
}

// task to send sensor data to Azure
void azure_task(void *pvParameter)
{
    int jsonLen;    
    time_t now;
    uint32_t heap, prevHeap;

    while (xSemaphoreTake(startUpMutex, (TickType_t) 10) != 1);

    ESP_LOGI(TAG, "Device to cloud interval is %d seconds.", deviceConfig.deviceToCloudInterval);
    
    // setup watchdog timer
    ESP_LOGI(TAG, "WDT initialized with %d seconds.", deviceConfig.watchdogTimeFence);
    task_wdt_set_timeout(deviceConfig.watchdogTimeFence);
    esp_task_wdt_add(NULL);
    io_expander_set_pin(SIGNAL_PIN_WDE, 1);

    // boot time stamp
    time(&now);
    deviceStatus.lastBootTime = now;

    // initialize sensor data struct
    init_sensor_data(&samplingMsg, deviceConfig.iotDeviceId, "data");

    // initialize status message struct
    init_devicve_status(&deviceStatus, deviceConfig.iotDeviceId, "status");
    
    // Create status message to send to Azure IoT
    enqueueStatusMsg(&deviceStatus, &samplingMsg, "Starting up.");

    xSemaphoreGive(startUpMutex);

    heap = esp_get_free_heap_size();


    while (1)
    {
        prevHeap = heap;


        //Process cloud to device messages
        process_azure_msgs(azure_client);
        vTaskDelay(ONE_SECOND_DELAY *2);

        // get sensor data readings in sampling message               
        read_sensor_data(&samplingMsg, deviceConfig);

        // Create message to send to Azure IoT
        jsonLen = serializeSamplingMessage(&samplingMsg, jsonSamplingMsg);
        
        ESP_LOGI(TAG, "Azure Msg: %s", jsonSamplingMsg);

        //Send message to Azure IoT Hub
        io_expander_signal_pin(SIGNAL_PIN_AZURE);

        int sendresult = send_azure_msgs(jsonSamplingMsg, azure_client);                
        if (sendresult == 0)
        {
            ESP_LOGI(TAG, "Message no. %ld queued to be sent to Azure, %d bytes", samplingMsg.msgNum, jsonLen);                        
        }
        else
        {
            ESP_LOGE(TAG, "Message Not Queued for Azure");
        }

        // reset watchdog timer
        io_expander_signal_pin(SIGNAL_PIN_WDT);
        esp_task_wdt_reset();

        heap = esp_get_free_heap_size();

        ESP_LOGW(TAG,"Current heap: %d, Delta heap after full cycle before send Azure message: %d", heap, heap - prevHeap); 
        
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
        while (xSemaphoreTake(oledDisplayMutex, (TickType_t) 10) !=1);        
        oled_write_data(&samplingMsg, &deviceStatus, currentOLEDScreen);        
        xSemaphoreGive(oledDisplayMutex);

        vTaskDelay(ONE_SECOND_DELAY *5);
    }
    
    vTaskDelete(NULL);  
}

// task to detect gestures on TOF proximity sensor VL6180X
void gesture_sensor_task(void *pvParameter)
{
    // wait for statup procedure to complete
    while (xSemaphoreTake(startUpMutex, (TickType_t) 10) != 1);

    // free mutex 
    xSemaphoreGive(startUpMutex);

    while(1)
    {        
        if (read_tof_gesture())
        {
            // gesture detected
            currentOLEDScreen++;
            if (currentOLEDScreen > OLED_SCREENS -1)
                currentOLEDScreen = 0;     

            ESP_LOGI(TAG, "TOF sensor gesture detected. Changing to screen %d.", currentOLEDScreen);   

            // update OLED display immediately
            while (xSemaphoreTake(oledDisplayMutex, (TickType_t) 10) !=1);
            oled_write_data(&samplingMsg, &deviceStatus, currentOLEDScreen);
            xSemaphoreGive(oledDisplayMutex);

            // wait until no gesture in progress              
            while (read_tof_gesture() == 1);
        }
        
        vTaskDelay(ONE_MILLISECOND_DELAY *200);
    }
    
    vTaskDelete(NULL);  
}

// task to detect pressed buttons on mcp23017
void button_press_task(void *pvParameter)
{
    uint8_t button_pressed = 0;

    // wait for statup procedure to complete
    while (xSemaphoreTake(startUpMutex, (TickType_t) 10) != 1);

    // free mutex 
    xSemaphoreGive(startUpMutex);

    while(1)
    {
        button_pressed = read_button_press();

        if (button_pressed > 0)
        {
            switch (button_pressed)
            {
                case BUTTON_HOME:
                    currentOLEDScreen = 0;     
                    break;
                case BUTTON_FWD:
                    currentOLEDScreen++;
                    if (currentOLEDScreen > OLED_SCREENS -1)
                        currentOLEDScreen = 0;     
                    break;
                case BUTTON_BCK:
                    if (currentOLEDScreen == 0)
                        currentOLEDScreen = OLED_SCREENS;     
                    currentOLEDScreen--;     
                    break;        
                default:
                    break;
            }

            // update OLED display immediately
            while (xSemaphoreTake(oledDisplayMutex, (TickType_t) 10) !=1);
            oled_write_data(&samplingMsg, &deviceStatus, currentOLEDScreen);
            xSemaphoreGive(oledDisplayMutex);
        }

        vTaskDelay(ONE_MILLISECOND_DELAY *200);
    }
    
    vTaskDelete(NULL);  
}


// app_main: main entry point
void app_main()
{
    int taskResult;    

    // set initial screen for OLED display
    currentOLEDScreen = 0;

    board_init();
    vTaskDelay(ONE_SECOND_DELAY);
    ESP_LOGI(TAG, "ESP32 board initialized. Device id is: %s", iotDeviceId);

    startUpMutex = xSemaphoreCreateMutex();
    oledDisplayMutex = xSemaphoreCreateMutex();

    int cnt = 0;
    while (!internetConnected)
    {   
        vTaskDelay(ONE_SECOND_DELAY * 5);  
        ESP_LOGI(TAG, "Waiting for IP adress %d", cnt++);   
        if (cnt > MAX_IP_RETRYS)
        {
            ESP_LOGE(TAG, "No IP adress received in time, rebooting chip in 5 sec..."); 
            vTaskDelay(ONE_SECOND_DELAY * 5);               
            esp_restart();
        }
    }

    azure_client = init_iothub_client(iotDeviceId);
    if (azure_client == NULL)
    {
        ESP_LOGE(TAG, "APP INFO: No Azure IoT Hub Handle was obtained");                
    }


    // set initial time type from last telemetry message.
    setStartupTimeType(&deviceConfig);
    
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

    // if gesture sensor is present on display shield, enable gesture detection task
    if (deviceHWStatus.vl6180x_present)
    {
        //Low Priority Task for capturing gesture data
        taskResult = xTaskCreate(&gesture_sensor_task, "gesture_sensor_task", 1024 * 3, NULL, 2, NULL); 
        if (taskResult != pdPASS)
        {   
            ESP_LOGE(TAG, "Create gesture sensor task failed"); 
        }
    }

    // if mcp23017 is present, display shiel has buttons to change display, enable button detection task
    if (deviceHWStatus.mcp23017d_present)
    {
        //Low Priority Task for capturing button presses
        taskResult = xTaskCreate(&button_press_task, "button_press_task", 1024 * 3, NULL, 2, NULL); 
        if (taskResult != pdPASS)
        {   
            ESP_LOGE(TAG, "Create button press task failed"); 
        }
    }

}
