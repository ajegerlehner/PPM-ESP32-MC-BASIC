#include "cJSON.h"
#include "mc_data.h"
#include "esp_log.h"

#define TAG "DATA"

uint32_t assign_json_tag_uint32(cJSON *pl, const char *tag_name);
uint8_t assign_json_tag_uint8(cJSON *pl, const char *tag_name);

int serializeSamplingMessage(SamplingMessage *msg, char *jsonString)
{
    char *serialized_string = NULL;
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    json_object_set_string(root_object, "iotDeviceId", msg->iotDeviceId);
    json_object_set_string(root_object, "msgType", msg->msgType);
    json_object_set_number(root_object, "timeStamp", msg->timeStamp);
    json_object_set_number(root_object, "timeType", msg->timeType);
    json_object_set_number(root_object, "elapsedTime", msg->elapseTime);
    json_object_set_number(root_object, "speed", msg->speed);
    json_object_set_number(root_object, "tickCount1", msg->tickCount1);
    json_object_set_number(root_object, "tickCount2", msg->tickCount2);
    json_object_set_number(root_object, "tickCount3", msg->tickCount3);
    json_object_set_number(root_object, "tickCount4", msg->tickCount4);
    json_object_set_number(root_object, "tickCount5", msg->tickCount5);
    json_object_set_number(root_object, "tickCount6", msg->tickCount6);
    json_object_set_number(root_object, "counterdelta1", msg->counterDelta1);
    json_object_set_number(root_object, "counterdelta2", msg->counterDelta2);
    json_object_set_number(root_object, "counterdelta3", msg->counterDelta3);
    json_object_set_number(root_object, "counterdelta4", msg->counterDelta4);
    json_object_set_number(root_object, "counterdelta5", msg->counterDelta5);
    json_object_set_number(root_object, "counterdelta6", msg->counterDelta6);
    json_object_set_number(root_object, "logicLevel1", msg->logicLevel1);
    json_object_set_number(root_object, "logicLevel2", msg->logicLevel2);
    json_object_set_number(root_object, "logicLevel3", msg->logicLevel3);
    json_object_set_number(root_object, "logicLevel4", msg->logicLevel4);
    json_object_set_number(root_object, "logicLevel5", msg->logicLevel5);
    json_object_set_number(root_object, "temperature", msg->temperature);
    json_object_set_number(root_object, "rtdTemperature", msg->rtd_temperature);
    json_object_set_number(root_object, "humidity", msg->humidity);
    json_object_set_number(root_object, "pressure", msg->pressure);
    json_object_set_number(root_object, "altitude", msg->altitude);
    json_object_set_number(root_object, "maxAmplitudeX", msg->maxAmplitudeX);
    json_object_set_number(root_object, "frequencyMaxX", msg->frequencyMaxX);
    json_object_set_number(root_object, "maxAmplitudeY", msg->maxAmplitudeY);
    json_object_set_number(root_object, "frequencyMaxY", msg->frequencyMaxY);
    json_object_set_number(root_object, "maxAmplitudeZ", msg->maxAmplitudeZ);
    json_object_set_number(root_object, "frequencyMaxZ", msg->frequencyMaxZ);
    json_object_set_number(root_object, "freeHeapSize", msg->freeHeapSize);

    serialized_string = json_serialize_to_string(root_value);
    strcpy(jsonString, serialized_string);

    // free json memory
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);

    return strlen(jsonString);
}

int serializeStatusgMessage(DeviceStatus *status, char *jsonString)
{
    char *serialized_string = NULL;
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    json_object_set_string(root_object, "iotDeviceId", status->iotDeviceId);
    json_object_set_string(root_object, "msgType", status->msgType);
    json_object_set_string(root_object, "statusMsg", status->statusMsg);
    json_object_set_string(root_object, "serialNo", status->serialNo);
    json_object_set_number(root_object, "timeStamp", status->timeStamp);
    json_object_set_number(root_object, "lastBootTime", status->lastBootTime);
    json_object_set_number(root_object, "upTime", status->upTime);
    json_object_set_number(root_object, "azureMsgSent", status->azureMsgSent);
    json_object_set_number(root_object, "azureMsgReceived", status->azureMsgReceived);    
    json_object_set_number(root_object, "processorTemperature", status->processorTemperature);
    json_object_set_number(root_object, "rtcBatteryLow", status->rtcBatteryLow);
    json_object_set_number(root_object, "hwRevision", status->hwRevision);
    json_object_set_number(root_object, "heapSize", status->heapSize);
    json_object_set_string(root_object, "appVersion", status->appVersion);
    json_object_set_number(root_object, "lastOTAUpdate", status->lastOTAUpdate);

    serialized_string = json_serialize_to_string(root_value);
    strcpy(jsonString, serialized_string);

    // free json memory
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);

    return strlen(jsonString);
}
