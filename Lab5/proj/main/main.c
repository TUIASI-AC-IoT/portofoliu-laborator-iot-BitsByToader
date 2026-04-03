/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "esp_http_server.h"


#include "lwip/err.h"
#include "lwip/sys.h"

#include "soft-ap.h"
#include "http-server.h"

#include "mdns.h"

#define MAX_APS 10
wifi_ap_record_t ap_info[MAX_APS]; 
uint16_t ap_count = 0;

char saved_ssid[32] = {0};
char saved_pass[64] = {0};

void provision_main(void)
{
  // TODO: 3. Pornire mod STA + scanare SSID-uri disponibile
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Pornim scanarea blocantă
    wifi_scan_config_t scan_config = {0};
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));     
    uint16_t number = MAX_APS;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ap_count = number;    
    ESP_LOGI("MAIN", "S-au gasit %d retele", ap_count);

    // Oprim modul STA pentru a porni AP-ul
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));

    // TODO: 4. Initializare mDNS (daca mai ramana timp)    
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("setup"));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 Provisioning Server"));

    // TODO: 1. Pornire softAP
    wifi_init_softap();

    // TODO: 2. Pornire server web (si config specifice in http-server.c) 
    start_webserver();
}

void normal_main(void)
{ 
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config = {0};
  strcpy((char *)wifi_config.sta.ssid, saved_ssid);
  strcpy((char *)wifi_config.sta.password, saved_pass);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  
  ESP_ERROR_CHECK(esp_wifi_connect());
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Preia din nvs wifi-ul daca exista.
    nvs_handle_t my_handle;
    size_t ssid_len = sizeof(saved_ssid);
    size_t pass_len = sizeof(saved_pass);
    
    // Încercăm să deschidem spațiul creat la pasul anterior (Read-Only)
    esp_err_t err = nvs_open("wifi", NVS_READONLY, &my_handle);
    esp_err_t err_ssid = nvs_get_str(my_handle, "ssid", saved_ssid, &ssid_len);
    esp_err_t err_pass = nvs_get_str(my_handle, "pass", saved_pass, &pass_len);
    nvs_close(my_handle);

    if (err == ESP_OK && err_ssid == ESP_OK && err_pass == ESP_OK) {
      // Avem wifi station
      // TODO: Daca nu reusim sa ne conectam, trecem in provision mode?
      ESP_LOGI("MAIN", "Date gasite in NVS! Ma conectez la: %s", saved_ssid);
      normal_main();
    } else {
      // nu avem wifi, cere de la user
      ESP_LOGI("MAIN", "Nicio retea salvata. Pornesc provisioning-ul...");
      provision_main();
    }
}