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
#include "esp_http_server.h"

#include "mc_web_server.h"

 #define TAG "WEBSRV"

 static esp_err_t on_default_uri(httpd_req_t *req)
 {
     char *msg = "<h1>PPM-MC-IOT</h1><p>Got to page %s to configure WiFi parameters.</p>";

     ESP_LOGI(TAG, "Web server url %s was hit.", req->uri);
     httpd_resp_send(req, msg, strlen(msg));
     return ESP_OK;
 }

 static esp_err_t on_wifi_uri(httpd_req_t *req)
 {
     char *msg = "<h1>PPM-MC-IOT</h1><p>Configuration page for WiFi parameters.</p>";

     ESP_LOGI(TAG, "Web server url %s was hit.", req->uri);
     httpd_resp_send(req, msg, strlen(msg));
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
     ESP_LOGI(TAG, "Stating web server.");
     ret = httpd_start(&server_handle, &server_config);
     if (ret != ESP_OK)
     {
         ESP_LOGE(TAG, "Error starting web server: Error code: %d", ret);
     }

     // define enpoints
     httpd_uri_t wifi_endpoint_config = {
         .uri = "/wifi",
         .method = HTTP_GET,
         .handler = on_wifi_uri
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