/**
 * Name:            mc_iotcentral.c
 * Description:     Hanlde connection to PPM IoT Central and get
 *                  device configuration from iot central
 * Initial Date:    2020-06-06
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include "mc_device.h"

#define TAG  "IOTCENT"

char *configBuffer = NULL;
int bufferIdx = 0;


esp_err_t clientEventHandler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP EVENT_ON_DATA length = %d", evt->data_len);
            if (configBuffer == NULL)
            {
               configBuffer = (char *) malloc(evt->data_len); 
               ESP_LOGW(TAG,"malloc configBuffer clientEventHandler"); //todo
            }
            else
            {
                configBuffer = (char *) realloc(configBuffer, evt->data_len + bufferIdx);
                ESP_LOGW(TAG,"realloc configBuffer clientEventHandler"); //todo
            }
            memcpy(&configBuffer[bufferIdx], evt->data, evt->data_len);
            bufferIdx += evt->data_len;            
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP EVENT_ON_FINISH length = %d", bufferIdx);
            configBuffer = (char *) realloc(configBuffer, bufferIdx+1);
            memcpy(&configBuffer[bufferIdx], "\0",1);
            ESP_LOGI(TAG, "%s", configBuffer);

            // deserialize device config 
            deserializeDeviceConfig(configBuffer);
            break;

        default:
            break;
    }

    return ESP_OK;
}
void get_device_config(char *deviceId)
{
    char body[200];
    
    memset(body, 0, sizeof(body));
    sprintf(body, "{\"Name\":\"%s\",\"Secret\":\"%s\",\"Serial\":\"12345678901234567890\"}", deviceId, CONFIG_PPM_IOT_CENTRAL_SECRET);
    
    // configure client config - http request
    esp_http_client_config_t clientConfig = {
        .url = CONFIG_PPM_IOT_CENTRAL_URL,
        .method = HTTP_METHOD_POST,         
        .event_handler = clientEventHandler,
        .crt_bundle_attach = esp_crt_bundle_attach
    };

    // create http client
    esp_http_client_handle_t client = esp_http_client_init(&clientConfig);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body, strlen(body));

    // call the web service
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP POST status = %d, content_length = %d",
                  esp_http_client_get_status_code(client),
                  esp_http_client_get_status_code(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));

        // no point to continue - restart
        ESP_LOGE(TAG, "HTTP POST request failed, restarting esp32");
        esp_restart();
    }
    
    // release client 
    esp_http_client_cleanup(client);
}

