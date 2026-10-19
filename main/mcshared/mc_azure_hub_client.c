/**
 * Name:            mc_azure_hub_client.c
 * Description:     Azure IoT Hub device client. Uses the Azure SDK for Embedded C
 *                  for topics / SAS tokens and the ESP-IDF MQTT client as transport.
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "mqtt_client.h"
#include "mbedtls/base64.h"
#include "psa/crypto.h"
#include "azure/az_core.h"
#include "azure/az_iot.h"
#include "parson.h"
#include "mc_azure_hub_client.h"
#include "mc_common.h"
#include "mc_iotcentral.h"
#include "mc_device.h"
#include "mc_ota.h"
#include "mc_sensor_data.h"
#include "mc_data.h"

#define TAG "AUZURE_IOT"

#define SAS_TOKEN_LIFETIME_SEC          3600
#define SAS_TOKEN_REFRESH_MARGIN_SEC    600
#define SAS_KEY_MAX_LEN                 64
#define MQTT_KEEPALIVE_SEC              240
#define MQTT_BUFFER_SIZE                2048
#define MQTT_OUTBOX_LIMIT               (64 * 1024)     // max bytes queued while offline
#define MQTT_TASK_STACK_SIZE            (1024 * 8)
#define MQTT_TOPIC_MAX_LEN              192
#define OTA_TASK_STACK_SIZE             (1024 * 8)
#define MAX_METHOD_NAME_LEN             20              // same as MAX_C2D_LEN in mc_sensor_data.c
#define METHOD_STATUS_OK                200
#define METHOD_STATUS_NOT_FOUND         404
#define METHOD_STATUS_ERROR             500

#ifdef CONFIG_PPM_IOT_MQTT_WEBSOCKETS
    #define IOTHUB_MQTT_TRANSPORT       MQTT_TRANSPORT_OVER_WSS
    #define IOTHUB_MQTT_PORT            443
    #define IOTHUB_MQTT_PATH            "/$iothub/websocket"
#else
    #define IOTHUB_MQTT_TRANSPORT       MQTT_TRANSPORT_OVER_SSL
    #define IOTHUB_MQTT_PORT            8883
    #define IOTHUB_MQTT_PATH            NULL
#endif

struct mc_iothub_client
{
    az_iot_hub_client hub;
    esp_mqtt_client_handle_t mqtt;
    char *hostName;
    char *deviceId;
    uint8_t sasKey[SAS_KEY_MAX_LEN];
    size_t sasKeyLen;
    char userName[256];
    char sasToken[320];
    time_t sasTokenExpiry;
    char telemetryTopic[MQTT_TOPIC_MAX_LEN];
    volatile bool connected;
    volatile bool authRejected;
    uint32_t requestId;
    char *rxTopic;      // incoming message being reassembled
    char *rxData;
};

typedef struct
{
    const char *method;
    TimeType timeType;
    const char *label;
    const char *statusMsg;
    const char *response;
} TimeTypeMethod;

// direct methods to force a time type (new operation on sfdc terminal)
static const TimeTypeMethod timeTypeMethods[] = {
    { "SETUP",      Setuptime,    "Setuptime",         "Cloud to device: SETUP.",      "{ \"Response\": \"New setup operation triggered.\" }" },
    { "RUNTIME",    Runtime,      "Runtime",           "Cloud to device: RUNTIME.",    "{ \"Response\": \"New runtime operation id triggered.\" }" },
    { "DOWNTIME",   Downtime,     "Downtime",          "Cloud to device: DOWNTIME.",   "{ \"Response\": \"New downtime operation id triggered.\" }" },
    { "PAUSE",      Pause,        "Pause",             "Cloud to device: PAUSE.",      "{ \"Response\": \"New pause operation id triggered.\" }" },
    { "AUXTIME",    Auxiliary,    "Auxiliary time",    "Cloud to device: AUXTIME.",    "{ \"Response\": \"New auxiliary time triggered.\" }" },
    { "UNPRODTIME", Unproductive, "Unproductive time", "Cloud to device: UNPRODTIME.", "{ \"Response\": \"New unproductive time triggered.\" }" },
};

extern SamplingMessage samplingMsg;
extern DeviceStatus deviceStatus;
void enqueueStatusMsg(DeviceStatus *statusMsg, SamplingMessage *samplingMsg, char *statusTxt);

DeviceTwin currentDeviceTwin;

static char currentSoftwareVersion[sizeof(((esp_app_desc_t *)0)->version)];
static RebootStruct reboot;
static volatile bool otaInProgress;

// returns a copy of the value of "<key>=" in a ';' separated connection string
static char *connection_string_value(const char *connectionString, const char *key)
{
    size_t keyLen = strlen(key);
    const char *pos = connectionString;

    while (pos != NULL)
    {
        if (strncmp(pos, key, keyLen) == 0 && pos[keyLen] == '=')
        {
            const char *value = pos + keyLen + 1;
            const char *end = strchr(value, ';');

            return strndup(value, end != NULL ? (size_t)(end - value) : strlen(value));
        }

        pos = strchr(pos, ';');
        if (pos != NULL)
        {
            pos++;
        }
    }

    return NULL;
}

// HMAC-SHA256 with a volatile PSA key, the key is not stored in NVS
static esp_err_t hmac_sha256(const uint8_t *key, size_t keyLen, const uint8_t *message, size_t messageLen, uint8_t *mac, size_t macSize)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t keyId;
    size_t macLen;
    psa_status_t status;

    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);

    status = psa_import_key(&attributes, key, keyLen, &keyId);
    psa_reset_key_attributes(&attributes);
    if (status != PSA_SUCCESS)
    {
        ESP_LOGE(TAG, "Failed to import SAS key: %d", (int)status);
        return ESP_FAIL;
    }

    status = psa_mac_compute(keyId, PSA_ALG_HMAC(PSA_ALG_SHA_256), message, messageLen, mac, macSize, &macLen);
    psa_destroy_key(keyId);
    if (status != PSA_SUCCESS || macLen != macSize)
    {
        ESP_LOGE(TAG, "Failed to compute SAS signature: %d", (int)status);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t generate_sas_token(struct mc_iothub_client *client)
{
    uint8_t signatureBuffer[256];
    uint8_t hmac[32];
    unsigned char hmacBase64[64];
    size_t hmacBase64Len;
    az_span signature;
    time_t expiry = time(NULL) + SAS_TOKEN_LIFETIME_SEC;

    if (az_result_failed(az_iot_hub_client_sas_get_signature(&client->hub, (uint64_t)expiry, AZ_SPAN_FROM_BUFFER(signatureBuffer), &signature)))
    {
        ESP_LOGE(TAG, "Failed to build SAS signature");
        return ESP_FAIL;
    }

    if (hmac_sha256(client->sasKey, client->sasKeyLen,
                    az_span_ptr(signature), (size_t)az_span_size(signature), hmac, sizeof(hmac)) != ESP_OK ||
        mbedtls_base64_encode(hmacBase64, sizeof(hmacBase64), &hmacBase64Len, hmac, sizeof(hmac)) != 0)
    {
        ESP_LOGE(TAG, "Failed to sign SAS token");
        return ESP_FAIL;
    }

    if (az_result_failed(az_iot_hub_client_sas_get_password(&client->hub, (uint64_t)expiry, az_span_create(hmacBase64, (int32_t)hmacBase64Len),
                                                            AZ_SPAN_EMPTY, client->sasToken, sizeof(client->sasToken), NULL)))
    {
        ESP_LOGE(TAG, "Failed to build SAS token");
        return ESP_FAIL;
    }

    client->sasTokenExpiry = expiry;

    return ESP_OK;
}

static void fill_mqtt_config(struct mc_iothub_client *client, esp_mqtt_client_config_t *mqttConfig)
{
    memset(mqttConfig, 0, sizeof(*mqttConfig));

    mqttConfig->broker.address.hostname = client->hostName;
    mqttConfig->broker.address.transport = IOTHUB_MQTT_TRANSPORT;
    mqttConfig->broker.address.port = IOTHUB_MQTT_PORT;
    mqttConfig->broker.address.path = IOTHUB_MQTT_PATH;
    mqttConfig->broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
    mqttConfig->credentials.client_id = client->deviceId;
    mqttConfig->credentials.username = client->userName;
    mqttConfig->credentials.authentication.password = client->sasToken;
    mqttConfig->session.keepalive = MQTT_KEEPALIVE_SEC;
    mqttConfig->buffer.size = MQTT_BUFFER_SIZE;
    mqttConfig->task.stack_size = MQTT_TASK_STACK_SIZE;
    mqttConfig->outbox.limit = MQTT_OUTBOX_LIMIT;
}

static void free_device_twin(DeviceTwin *twin)
{
    free(twin->currentSoftwareVersion);
    free(twin->desiredSoftwareVersion);
    free(twin->softwareFilePath);
    free(twin);
}

char* deviceTwinToJSON(DeviceTwin* twin)
{
    char* jsonStr;
    JSON_Value* root_value = json_value_init_object();
    JSON_Object* root_object = json_value_get_object(root_value);

    // send only reported properties
    json_object_set_string(root_object, "firmwareVersion", twin->currentSoftwareVersion);

    jsonStr = json_serialize_to_string(root_value);

    json_value_free(root_value);

    return jsonStr;
}

DeviceTwin* deviceTwinfromJSON(const char* jsonStr, bool completeTwin)
{
    DeviceTwin* deviceTwin = malloc(sizeof(DeviceTwin));
    JSON_Value* root_value = NULL;
    JSON_Object* root_object = NULL;
    const char* firmwareVersion;
    const char* firmwareFilePath;

    if (deviceTwin == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for device twin.");
        return NULL;
    }

    memset(deviceTwin, 0, sizeof(DeviceTwin));

    root_value = json_parse_string(jsonStr);
    root_object = json_value_get_object(root_value);

    // get only desired properties
    if (completeTwin)
    {
        ESP_LOGI(TAG, "Device Twin update complete...");
        firmwareVersion = json_object_dotget_string(root_object, "desired.firmware.firmwareVersion");
        firmwareFilePath = json_object_dotget_string(root_object, "desired.firmware.url");
    }
    else
    {
        ESP_LOGI(TAG, "Device Twin update partial...");
        firmwareVersion = json_object_dotget_string(root_object, "firmware.firmwareVersion");
        firmwareFilePath = json_object_dotget_string(root_object, "firmware.url");
    }

    if (firmwareVersion != NULL)
    {
        deviceTwin->desiredSoftwareVersion = strdup(firmwareVersion);
        if (deviceTwin->desiredSoftwareVersion == NULL)
        {
            ESP_LOGE(TAG, "Failed allocating memory for desired property desiredSoftwareVersion");
        }
    }
    if (firmwareFilePath != NULL)
    {
        deviceTwin->softwareFilePath = strdup(firmwareFilePath);
        if (deviceTwin->softwareFilePath == NULL)
        {
            ESP_LOGE(TAG, "Failed allocating memory for desired property desiredSoftwareFilePath");
        }
    }

    deviceTwin->currentSoftwareVersion = strdup(currentDeviceTwin.currentSoftwareVersion);
    if (deviceTwin->currentSoftwareVersion == NULL)
    {
        ESP_LOGE(TAG, "Failed allocating memory for desired property currentSoftwareVersion");
    }

    json_value_free(root_value);

    return deviceTwin;
}

// OTA download runs in its own task so the MQTT connection is kept alive meanwhile
static void ota_task(void *pvParameter)
{
    DeviceTwin *newTwin = (DeviceTwin *)pvParameter;

    // perform OTA update from Azure blob storage
    performOTAUpdate(newTwin);

    // remember the version, so the same desired version does not trigger another update
    strlcpy(currentSoftwareVersion, newTwin->desiredSoftwareVersion, sizeof(currentSoftwareVersion));

    free_device_twin(newTwin);
    otaInProgress = false;

    vTaskDelete(NULL);
}

static void deviceTwinCallback(bool completeTwin, const char *payload)
{
    DeviceTwin* newTwin = deviceTwinfromJSON(payload, completeTwin);

    if (newTwin == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse device twin from JSON.");
        return;
    }

    if (newTwin->desiredSoftwareVersion == NULL || newTwin->currentSoftwareVersion == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse new twin JSON desired firmwareVersion. Skipping update.");
        free_device_twin(newTwin);
        return;
    }

    ESP_LOGI(TAG, "Device Twin callback. New twin firmwareVersion: %s, current firmwareVersion is %s",  newTwin->desiredSoftwareVersion, newTwin->currentSoftwareVersion);

    if (strcmp(newTwin->currentSoftwareVersion, newTwin->desiredSoftwareVersion) != 0)
    {
        // new desired firmware version reveived, prepare for OTA update
        ESP_LOGI(TAG, "Reveiced new firmware version: %s, current version is %s.", newTwin->desiredSoftwareVersion, newTwin->currentSoftwareVersion);

        if (otaInProgress)
        {
            ESP_LOGW(TAG, "OTA update already in progress. Skipping update.");
        }
        else
        {
            otaInProgress = true;
            if (xTaskCreate(&ota_task, "ota_task", OTA_TASK_STACK_SIZE, newTwin, 5, NULL) == pdPASS)
            {
                // ota task owns the twin now
                return;
            }

            otaInProgress = false;
            ESP_LOGE(TAG, "Create OTA task failed");
        }
    }

    free_device_twin(newTwin);
}

// requests the full twin document and reports the running firmware version
static void sync_device_twin(struct mc_iothub_client *client)
{
    char topic[MQTT_TOPIC_MAX_LEN];
    char requestId[12];
    char *reportedProperties;

    snprintf(requestId, sizeof(requestId), "%" PRIu32, ++client->requestId);
    if (az_result_succeeded(az_iot_hub_client_twin_document_get_publish_topic(&client->hub, az_span_create_from_str(requestId), topic, sizeof(topic), NULL)))
    {
        esp_mqtt_client_enqueue(client->mqtt, topic, "", 0, 1, 0, true);
    }

    reportedProperties = deviceTwinToJSON(&currentDeviceTwin);
    if (reportedProperties != NULL)
    {
        snprintf(requestId, sizeof(requestId), "%" PRIu32, ++client->requestId);
        if (az_result_succeeded(az_iot_hub_client_twin_patch_get_publish_topic(&client->hub, az_span_create_from_str(requestId), topic, sizeof(topic), NULL)))
        {
            esp_mqtt_client_enqueue(client->mqtt, topic, reportedProperties, 0, 1, 0, true);
        }

        json_free_serialized_string(reportedProperties);
    }
}

static void handle_twin_response(const az_iot_hub_client_twin_response *response, const char *payload)
{
    switch (response->response_type)
    {
        case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET:
            if (response->status == AZ_IOT_STATUS_OK)
            {
                deviceTwinCallback(true, payload);
            }
            else
            {
                ESP_LOGE(TAG, "Device Twin request failed with status: %d", response->status);
            }
            break;

        case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
            deviceTwinCallback(false, payload);
            break;

        case AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
            ESP_LOGI(TAG, "DeviceTwin reported properties update completed with status: %d. Reported firmware version: %s", response->status, currentDeviceTwin.currentSoftwareVersion);
            break;

        default:
            ESP_LOGE(TAG, "Device Twin request error, status: %d", response->status);
            break;
    }
}

// handles a direct method, methodName is upper case. Returns the method status code.
static int deviceMethodCallback(const char *methodName, const char *payload, const char **response)
{
    if (strcmp("REBOOT", methodName) == 0)
    {
        enqueueStatusMsg(&deviceStatus, &samplingMsg, "Cloud to device: REBOOT.");
        *response = "{ \"Response\": \"Rebooting chip.\" }";

        // spawn reboot task
        reboot.rebootDelay = CONFIG_PPM_IOT_REBOOT_DELAY;
        spawn_reboot_task(&reboot);

        return METHOD_STATUS_OK;
    }

    if (strcmp("NVSCLEAR", methodName) == 0)
    {
        if (nvs_delete_key(NVS_SSID_TAG) == ESP_OK &&
            nvs_delete_key(NVS_SSID_PWD_TAG) == ESP_OK &&
            nvs_delete_key(NVS_NODENAME_TAG) == ESP_OK)
        {
            *response = "{ \"Response\": \"WiFi keys erased.\" }";

            enqueueStatusMsg(&deviceStatus, &samplingMsg, "Cloud to device: NVSCLEAR.");

            // spawn reboot task
            reboot.rebootDelay = 10;
            spawn_reboot_task(&reboot);

            return METHOD_STATUS_OK;
        }

        *response = "{ \"Response\": \"Error erasing WiFi keys.\" }";

        return METHOD_STATUS_ERROR;
    }

    if (strcmp("UPDATE", methodName) == 0)
    {
        enqueueStatusMsg(&deviceStatus, &samplingMsg, "Cloud to device: UPDATE.");
        *response = "{ \"Response\": \"Update message triggered.\" }";

        return METHOD_STATUS_OK;
    }

    if (strcmp("STATUS", methodName) == 0)
    {
        enqueueStatusMsg(&deviceStatus, &samplingMsg, "Cloud to device: STATUS.");
        *response = "{ \"Response\": \"Update status message triggered.\" }";

        return METHOD_STATUS_OK;
    }

    for (size_t i = 0; i < sizeof(timeTypeMethods) / sizeof(timeTypeMethods[0]); i++)
    {
        if (strcmp(timeTypeMethods[i].method, methodName) == 0)
        {
            setCurrentTimeType(timeTypeMethods[i].timeType);
            enqueueStatusMsg(&deviceStatus, &samplingMsg, (char *)timeTypeMethods[i].statusMsg);
            ESP_LOGI(TAG, "%s forced set by cloud to device message. Payload: %s", timeTypeMethods[i].label, payload);
            *response = timeTypeMethods[i].response;

            return METHOD_STATUS_OK;
        }
    }

    ESP_LOGW(TAG, "Unknown cloud to device message received.");
    enqueueStatusMsg(&deviceStatus, &samplingMsg, "Cloud to device: UNKNOWN.");
    // All other entries are ignored.
    *response = "{ \"Response\": \"Unknown method call.\" }";

    return METHOD_STATUS_NOT_FOUND;
}

static void handle_method_request(struct mc_iothub_client *client, const az_iot_hub_client_method_request *request, const char *payload)
{
    char methodName[MAX_METHOD_NAME_LEN + 1];
    char topic[MQTT_TOPIC_MAX_LEN];
    const char *response = NULL;
    int status;

    // names longer than any known method are reported as unknown
    snprintf(methodName, sizeof(methodName), "%.*s", (int)az_span_size(request->name), (const char *)az_span_ptr(request->name));

    // log cloud to device messge in device status
    deviceStatus.azureMsgReceived++;
    if (deviceStatus.lastCloudToDeviceMsg != NULL)
    {
        strcpy(deviceStatus.lastCloudToDeviceMsg, methodName);
    }

    if (az_span_size(request->name) > MAX_METHOD_NAME_LEN)
    {
        methodName[0] = '\0';
    }
    for (char *c = methodName; *c != '\0'; c++)
    {
        *c = (char)toupper((unsigned char)*c);
    }

    status = deviceMethodCallback(methodName, payload, &response);

    if (az_result_succeeded(az_iot_hub_client_methods_response_get_publish_topic(&client->hub, request->request_id, (uint16_t)status, topic, sizeof(topic), NULL)))
    {
        esp_mqtt_client_enqueue(client->mqtt, topic, response, 0, 0, 0, true);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to build direct method response topic");
    }
}

static void dispatch_message(struct mc_iothub_client *client, const char *topic, const char *payload)
{
    az_span topicSpan = az_span_create_from_str((char *)topic);
    az_iot_hub_client_method_request methodRequest;
    az_iot_hub_client_twin_response twinResponse;

    if (az_result_succeeded(az_iot_hub_client_methods_parse_received_topic(&client->hub, topicSpan, &methodRequest)))
    {
        handle_method_request(client, &methodRequest, payload);
    }
    else if (az_result_succeeded(az_iot_hub_client_twin_parse_received_topic(&client->hub, topicSpan, &twinResponse)))
    {
        handle_twin_response(&twinResponse, payload);
    }
    else
    {
        ESP_LOGW(TAG, "Message on unexpected topic received: %s", topic);
    }
}

// large messages are delivered in chunks, collect them before dispatching
static void handle_mqtt_data(struct mc_iothub_client *client, esp_mqtt_event_handle_t event)
{
    if (event->current_data_offset == 0)
    {
        free(client->rxTopic);
        free(client->rxData);
        client->rxTopic = strndup(event->topic, event->topic_len);
        client->rxData = malloc(event->total_data_len + 1);
    }

    if (client->rxTopic == NULL || client->rxData == NULL ||
        event->current_data_offset + event->data_len > event->total_data_len)
    {
        ESP_LOGE(TAG, "Failed to receive message, %d bytes", event->total_data_len);
        return;
    }

    memcpy(client->rxData + event->current_data_offset, event->data, event->data_len);
    if (event->current_data_offset + event->data_len < event->total_data_len)
    {
        return;
    }

    client->rxData[event->total_data_len] = '\0';
    dispatch_message(client, client->rxTopic, client->rxData);

    free(client->rxTopic);
    free(client->rxData);
    client->rxTopic = NULL;
    client->rxData = NULL;
}

static void mqtt_event_handler(void *handlerArgs, esp_event_base_t base, int32_t eventId, void *eventData)
{
    struct mc_iothub_client *client = (struct mc_iothub_client *)handlerArgs;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)eventData;

    switch ((esp_mqtt_event_id_t)eventId)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "AZURE INFO: Connected to IoT Hub %s", client->hostName);
            client->connected = true;
            client->authRejected = false;

            esp_mqtt_client_subscribe(client->mqtt, AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC, 1);
            esp_mqtt_client_subscribe(client->mqtt, AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC, 1);
            esp_mqtt_client_subscribe(client->mqtt, AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC, 1);

            sync_device_twin(client);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "AZURE INFO: Disconnected from IoT Hub");
            client->connected = false;
            break;

        case MQTT_EVENT_DATA:
            handle_mqtt_data(client, event);
            break;

        case MQTT_EVENT_ERROR:
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
            {
                ESP_LOGE(TAG, "AZURE INFO: IoT Hub refused connection, return code %d", event->error_handle->connect_return_code);
                if (event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_BAD_USERNAME ||
                    event->error_handle->connect_return_code == MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED)
                {
                    // SAS token is renewed in process_azure_msgs
                    client->authRejected = true;
                }
            }
            else
            {
                ESP_LOGE(TAG, "AZURE INFO: MQTT transport error, esp_tls error 0x%x, tls stack error 0x%x",
                         event->error_handle->esp_tls_last_esp_err, event->error_handle->esp_tls_stack_err);
            }
            break;

        default:
            break;
    }
}

mc_iothub_handle_t init_iothub_client(char *deviceId)
{
    DeviceConfig devConfig;
    esp_mqtt_client_config_t mqttConfig;
    char *sasKeyBase64 = NULL;
    uint8_t propertyBuffer[64];
    az_iot_message_properties telemetryProperties;
    struct mc_iothub_client *client = calloc(1, sizeof(struct mc_iothub_client));

    if (client == NULL)
    {
        ESP_LOGE(TAG,"AZURE INFO: Error initializing Azure IoT Client");
        return NULL;
    }

    // a failed precondition is a programming error, reboot instead of the default endless loop
    az_precondition_failed_set_callback(abort);

    // SAS tokens need the current time
    wait_for_network_time();

    // get the device config from ppm iot central and provide connection string etc.
    get_device_config(deviceId);
    ESP_LOGI(TAG, "IoT Central device config loaded for device %s", deviceId);

    // get device configuration struct for connection string
    devConfig = get_device_config_data();
    if (devConfig.deviceConnectionString != NULL)
    {
        client->hostName = connection_string_value(devConfig.deviceConnectionString, "HostName");
        client->deviceId = connection_string_value(devConfig.deviceConnectionString, "DeviceId");
        sasKeyBase64 = connection_string_value(devConfig.deviceConnectionString, "SharedAccessKey");
    }

    if (client->hostName == NULL || client->deviceId == NULL || sasKeyBase64 == NULL ||
        mbedtls_base64_decode(client->sasKey, sizeof(client->sasKey), &client->sasKeyLen, (const unsigned char *)sasKeyBase64, strlen(sasKeyBase64)) != 0)
    {
        ESP_LOGE(TAG, "AZURE INFO: Invalid device connection string");
        goto error;
    }
    ESP_LOGI(TAG, "IoT Hub is %s, device id is %s", client->hostName, client->deviceId);

    // running firmware version is reported in the device twin
    strlcpy(currentSoftwareVersion, esp_app_get_description()->version, sizeof(currentSoftwareVersion));
    currentDeviceTwin.currentSoftwareVersion = currentSoftwareVersion;
    ESP_LOGI(TAG, "Setting up Azure Device Twin. App project is %s, App version is %s", esp_app_get_description()->project_name, currentSoftwareVersion);

    if (az_result_failed(az_iot_hub_client_init(&client->hub, az_span_create_from_str(client->hostName), az_span_create_from_str(client->deviceId), NULL)) ||
        az_result_failed(az_iot_hub_client_get_user_name(&client->hub, client->userName, sizeof(client->userName), NULL)) ||
        generate_sas_token(client) != ESP_OK)
    {
        ESP_LOGE(TAG,"AZURE INFO: Error initializing Azure IoT Client");
        goto error;
    }

    // telemetry is sent as utf-8 json, so it can be routed by message body
    if (az_result_failed(az_iot_message_properties_init(&telemetryProperties, AZ_SPAN_FROM_BUFFER(propertyBuffer), 0)) ||
        az_result_failed(az_iot_message_properties_append(&telemetryProperties, AZ_SPAN_FROM_STR(AZ_IOT_MESSAGE_PROPERTIES_CONTENT_TYPE), AZ_SPAN_FROM_STR("application%2Fjson"))) ||
        az_result_failed(az_iot_message_properties_append(&telemetryProperties, AZ_SPAN_FROM_STR(AZ_IOT_MESSAGE_PROPERTIES_CONTENT_ENCODING), AZ_SPAN_FROM_STR("utf-8"))) ||
        az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(&client->hub, &telemetryProperties, client->telemetryTopic, sizeof(client->telemetryTopic), NULL)))
    {
        ESP_LOGE(TAG,"AZURE INFO: Error initializing Azure IoT Client");
        goto error;
    }

    fill_mqtt_config(client, &mqttConfig);
    client->mqtt = esp_mqtt_client_init(&mqttConfig);
    if (client->mqtt == NULL ||
        esp_mqtt_client_register_event(client->mqtt, MQTT_EVENT_ANY, mqtt_event_handler, client) != ESP_OK ||
        esp_mqtt_client_start(client->mqtt) != ESP_OK)
    {
        ESP_LOGE(TAG,"AZURE INFO: Error initializing Azure IoT Client");
        goto error;
    }

    free(sasKeyBase64);

    return client;

error:
    free(sasKeyBase64);
    destroy_iothub_client(client);

    return NULL;
}

int send_azure_msgs(char *msgToSend, mc_iothub_handle_t iothub_deviceclient)
{
    if (iothub_deviceclient == NULL)
    {
        ESP_LOGE(TAG, "AZURE INFO: ERROR-IoT Hub Client Handle is NULL");
        return -1;
    }

    // message is stored and sent by the mqtt task, also when currently disconnected
    if (esp_mqtt_client_enqueue(iothub_deviceclient->mqtt, iothub_deviceclient->telemetryTopic, msgToSend, 0, 1, 0, true) < 0)
    {
        ESP_LOGE(TAG, "AZURE INFO: ERROR-Failed Azure Send Event");
        return -1;
    }

    ESP_LOGI(TAG, "AZURE INFO: Completed Message successfully accepted to Azure transmission");

    return 0;
}

void process_azure_msgs(mc_iothub_handle_t iothub_deviceclient)
{
    esp_mqtt_client_config_t mqttConfig;

    if (iothub_deviceclient == NULL)
    {
        return;
    }

    if (!iothub_deviceclient->authRejected &&
        time(NULL) < iothub_deviceclient->sasTokenExpiry - SAS_TOKEN_REFRESH_MARGIN_SEC)
    {
        if (!iothub_deviceclient->connected)
        {
            ESP_LOGW(TAG, "AZURE INFO: Not connected to IoT Hub, messages are queued");
        }
        return;
    }

    // IoT Hub drops the connection when the SAS token expires, reconnect with a new one
    ESP_LOGI(TAG, "AZURE INFO: Renewing SAS token");
    if (generate_sas_token(iothub_deviceclient) != ESP_OK)
    {
        return;
    }

    fill_mqtt_config(iothub_deviceclient, &mqttConfig);
    esp_mqtt_client_stop(iothub_deviceclient->mqtt);
    iothub_deviceclient->connected = false;
    iothub_deviceclient->authRejected = false;
    esp_mqtt_set_config(iothub_deviceclient->mqtt, &mqttConfig);
    esp_mqtt_client_start(iothub_deviceclient->mqtt);
}

void destroy_iothub_client(mc_iothub_handle_t iothub_deviceclient)
{
    if (iothub_deviceclient == NULL)
    {
        return;
    }

    ESP_LOGI(TAG, "AZURE INFO: Cleaning up - Removing Device Client");
    if (iothub_deviceclient->mqtt != NULL)
    {
        esp_mqtt_client_destroy(iothub_deviceclient->mqtt);
    }

    free(iothub_deviceclient->rxTopic);
    free(iothub_deviceclient->rxData);
    free(iothub_deviceclient->hostName);
    free(iothub_deviceclient->deviceId);
    free(iothub_deviceclient);
}
