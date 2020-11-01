/**
 * Name:            mc_web_server.c
 * Description:     Creates web service to setup wifi ssid and pwd
 * Initial Date:    2020-10-31
 * Author:          Andreas Jegerlehner
 * Copyright:       (c) Print & Packaging GmbH. All rights reserved.
**/

#include "stdio.h"
#include "string.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"

#include "mc_common.h"
#include "mc_web_server.h"

 #define TAG "WEBSRV"

 static esp_err_t on_default_uri(httpd_req_t *req)
 {
     char path[960];
     esp_vfs_spiffs_conf_t spiffs_config = {
         .base_path = "/spiffs",
         .partition_label = NULL,
         .max_files = 5,
         .format_if_mount_failed = false
     };

     ESP_LOGI(TAG, "Default endpoint: web server url %s was hit.", req->uri);

    // serve html pages from spiffs flash storage
     esp_vfs_spiffs_register(&spiffs_config);

    // set path to html file in spiffs depending on uri
    if (strcmp(req->uri, "/") == 0)
    {
        strcpy(path, "/spiffs/index.html");
    }
    else
    {
        sprintf(path, "/spiffs%s", req->uri);
    }

    //style.css
    char *ptr = strrchr(path, '.');
    if (strcmp(ptr, ".css") == 0)
    {
        ESP_LOGI(TAG, "Service css - setting mime type to css");
        httpd_resp_set_type(req, "text/css");
    }

    // open file
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        httpd_resp_send_404(req);
    }
    else
    {
        // serve the page
        char lineRead[256];
        while (fgets(lineRead, sizeof(lineRead), file))
        {
            httpd_resp_sendstr_chunk(req, lineRead);
        }
        httpd_resp_sendstr_chunk(req, NULL);              
    }
        


     esp_vfs_spiffs_unregister(NULL);
     return ESP_OK;
 }

 static esp_err_t on_wifi_set_uri(httpd_req_t *req)
 {
     char buf[150];
     BaseType_t taskResult;
     
     ESP_LOGI(TAG, "WiFi set endpoint: web server url %s was hit. Content length: %d", req->uri, req->content_len);
     
     memset(&buf, 0, sizeof(buf));
     httpd_req_recv(req, buf, req->content_len);

     ESP_LOGI(TAG, "Buffer received: %s", buf);

    // extract HTTP_POST tokens
     char *nodename = strtok(buf, "\r\n");
     char *ssid = strtok(NULL, "\r\n");
     char *password = strtok(NULL, "\r\n");

    // extract token value
    nodename = strchr(nodename, '=') + 1;
    ssid = strchr(ssid, '=') + 1;
    password = strchr(password, '=') + 1;

    // save values to nvs flash
    nvs_write_str_value(NVS_SSID_TAG, ssid);
    nvs_write_str_value(NVS_SSID_PWD_TAG, password);
    nvs_write_str_value(NVS_NODENAME_TAG, nodename);

    ESP_LOGI(TAG, "Flashed new WiFi parameters to nvs: ssid: %s, password: %s, nade name: %s", ssid, password, nodename);

    // redirect to response page
    httpd_resp_set_status(req, "303");
    httpd_resp_set_hdr(req, "Location", "/config-wifi-resp.html");
    httpd_resp_send(req, NULL, 0);

    // restart chip
    // spawn reboot task
    taskResult = xTaskCreate(&reboot_task, "reboot_task", 1024, NULL, 9, NULL); 
    if (taskResult != pdPASS)
    {   
        ESP_LOGE(TAG, "Create reboot task failed"); 
    }

     return ESP_OK;
 }

 void register_endpoints(void)
 {
     esp_err_t ret;
     httpd_handle_t server_handle = NULL;
     httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();

     // allow wildcard rouitng
     server_config.uri_match_fn = httpd_uri_match_wildcard;

    // start web server
     ESP_LOGI(TAG, "Starting web server.");
     ret = httpd_start(&server_handle, &server_config);
     if (ret != ESP_OK)
     {
         ESP_LOGE(TAG, "Error starting web server: Error code: %d", ret);
     }

     // define enpoints
     httpd_uri_t wifi_endpoint_config = {
         .uri = "/wifi-set",
         .method = HTTP_POST,
         .handler = on_wifi_set_uri
     };

     httpd_register_uri_handler(server_handle, &wifi_endpoint_config);

    // catch all uri
    httpd_uri_t default_endpoint_config = {
         .uri = "/*",
         .method = HTTP_GET,
         .handler = on_default_uri
     };

     httpd_register_uri_handler(server_handle, &default_endpoint_config);

 }