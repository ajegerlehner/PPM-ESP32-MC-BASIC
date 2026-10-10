# Azure SDK for Embedded C (https://github.com/Azure/azure-sdk-for-c), pulled in
# as a git submodule. Only the core and IoT Hub client sources are built; MQTT
# and TLS are provided by ESP-IDF (esp-mqtt / esp-tls).

set(AZ_SDK "${CMAKE_CURRENT_LIST_DIR}/azure-sdk-for-c/sdk")

if(NOT EXISTS "${AZ_SDK}/inc/azure/az_iot.h")
    message(FATAL_ERROR "azure-sdk-for-c submodule is missing. Run: git submodule update --init")
endif()

idf_component_register(
    SRCS "${AZ_SDK}/src/azure/core/az_base64.c"
         "${AZ_SDK}/src/azure/core/az_context.c"
         "${AZ_SDK}/src/azure/core/az_json_reader.c"
         "${AZ_SDK}/src/azure/core/az_json_token.c"
         "${AZ_SDK}/src/azure/core/az_json_writer.c"
         "${AZ_SDK}/src/azure/core/az_log.c"
         "${AZ_SDK}/src/azure/core/az_precondition.c"
         "${AZ_SDK}/src/azure/core/az_span.c"
         "${AZ_SDK}/src/azure/iot/az_iot_common.c"
         "${AZ_SDK}/src/azure/iot/az_iot_hub_client.c"
         "${AZ_SDK}/src/azure/iot/az_iot_hub_client_c2d.c"
         "${AZ_SDK}/src/azure/iot/az_iot_hub_client_methods.c"
         "${AZ_SDK}/src/azure/iot/az_iot_hub_client_sas.c"
         "${AZ_SDK}/src/azure/iot/az_iot_hub_client_telemetry.c"
         "${AZ_SDK}/src/azure/iot/az_iot_hub_client_twin.c"
         "${AZ_SDK}/src/azure/platform/az_noplatform.c"
    INCLUDE_DIRS "${AZ_SDK}/inc"
    PRIV_INCLUDE_DIRS "${AZ_SDK}/src/azure/core"
)
