set(COMPONENT_SRCS "main.c"
                    "mc_azure_hub_client.c"
                    "mc_common.c"
                    "mc_data.c"
                    "mc_oled_display.c" 
                    "mc_iotcentral.c" 
                    "mc_device.c" 
                    "mc_gpio.c" 
                    "mc_i2c_bus.c"
                    "mc_mcp23017.c"
                    "mc_stm32f04.c"
                    "mc_sensor_data.c"
                    "bme280.c"
                    "mc_ds3231.c"
                    "mc_eth.c"
                    "mc_iis328dq.c"
                    "mc_ota.c")

set(COMPONENT_EMBED_TXTFILES "../cert/azure.cer")

list(APPEND COMPONENT_ADD_INCLUDEDIRS "." 
                                      "include"
                                      "../components")

register_component()