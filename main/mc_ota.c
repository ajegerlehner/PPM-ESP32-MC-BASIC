/**
 * Name:            mc_ota.c
 * Description:     Handle over the air update process
 * Initial Date:    2020-10-25
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include "stdio.h"
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mc_azure_hub_client.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "mc_ota.h"
#include "mc_data.h"
#include "mc_sensor_data.h"
#include "mc_common.h"

#define RESTART_DELAY 10000

static const char* TAG = "OTA";
extern const uint8_t server_cert_pem_start[] asm("_binary_azure_cer_start");
extern DeviceStatus deviceStatus;
extern SamplingMessage samplingMsg;

esp_err_t client_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}

void performOTAUpdate(DeviceTwin* devTwin)
{
    time_t otaTime;    
    esp_http_client_config_t clientConfig = {
        .url = devTwin->softwareFilePath,
        .event_handler = client_event_handler,
        .cert_pem = (char *)server_cert_pem_start
    };

    ESP_LOGI(TAG, "Stating OTA update from version %s to version %s using image file %s", devTwin->currentSoftwareVersion, devTwin->desiredSoftwareVersion, devTwin->softwareFilePath);

    if (esp_https_ota(&clientConfig) == ESP_OK)
    {
        // ota update succeeded
        ESP_LOGI(TAG, "OTA update successfull to version %s - restarting in %d seconds.", devTwin->desiredSoftwareVersion, RESTART_DELAY / 1000);
        create_status_msg(&deviceStatus, &samplingMsg, "OTA Update success");
        time(&otaTime);
        nvs_write_uint32_value("lastOTAUpdate", otaTime);
        vTaskDelay(pdMS_TO_TICKS(RESTART_DELAY));
        esp_restart();
    }

    // ota update was not successfull
    ESP_LOGE(TAG, "Failed to perform OTA update to version %s, image file %s", devTwin->desiredSoftwareVersion, devTwin->softwareFilePath);
    create_status_msg(&deviceStatus, &samplingMsg, "OTA Update failed");
}

