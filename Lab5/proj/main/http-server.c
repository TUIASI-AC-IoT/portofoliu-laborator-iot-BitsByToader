#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "freertos/event_groups.h"

#include "esp_http_server.h"

#define MAX_APS 10
extern wifi_ap_record_t ap_info[MAX_APS];
extern uint16_t ap_count;

/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req)
{
    char *html_buffer = malloc(2048); // TODO: Buffer size?
    if (html_buffer == NULL) {
        ESP_LOGE("HTTP", "Nu s-a putut aloca memoria pentru HTML");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Antetul paginii
    strcpy(html_buffer, "<html><head><meta charset=\"UTF-8\"></head><body style=\"font-family: Arial; padding: 20px;\">"
                        "<h2>Configurare Dispozitiv IoT</h2>"
                        "<form action=\"/results.html\" method=\"post\">"
                        "<label for=\"ssid\">Alege reteaua Wi-Fi:</label><br>"
                        "<select name=\"ssid\" style=\"margin-bottom: 10px; width: 200px;\">");

    // Lista retele
    char option_buffer[128];
    for (int i = 0; i < ap_count; i++) {
        snprintf(option_buffer, sizeof(option_buffer),
                 "<option value=\"%s\">%s (Semnal: %d dBm)</option>",
                 ap_info[i].ssid, ap_info[i].ssid, ap_info[i].rssi);
        
        strcat(html_buffer, option_buffer);
    }

    strcat(html_buffer, "</select><br><br>"
                        "<label for=\"ipass\">Parola retelei:</label><br>"
                        "<input type=\"password\" name=\"ipass\" style=\"margin-bottom: 10px; width: 200px;\"><br><br>"
                        "<input type=\"submit\" value=\"Conecteaza\">"
                        "</form></body></html>");

    httpd_resp_send(req, html_buffer, HTTPD_RESP_USE_STRLEN);
    
    free(html_buffer);
    return ESP_OK;
}

/* Our URI handler function to be called during POST /uri request */
esp_err_t post_handler(httpd_req_t *req)
{
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    content[ret] = '\0'; // Asigurăm terminarea șirului de caractere primit

    char ssid[32] = {0};
    char pass[64] = {0};
    
    // Extragem SSID-ul si Parola introduse [cite: 180]
    if (httpd_query_key_value(content, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
        httpd_query_key_value(content, "ipass", pass, sizeof(pass)) == ESP_OK) {
        ESP_LOGI("HTTP", "Am primit SSID: %s si Parola: %s", ssid, pass);
    
        // TODO: Nu e chiar cel mai optim loc de a face asta. Ar trb in app main.
        nvs_handle_t my_handle;
        esp_err_t err = nvs_open("wifi", NVS_READWRITE, &my_handle);
        if (err == ESP_OK) {
            nvs_set_str(my_handle, "ssid", ssid);
            nvs_set_str(my_handle, "pass", pass);
            nvs_commit(my_handle); // actual save
            nvs_close(my_handle);
            ESP_LOGI("HTTP", "Datele au fost salvate cu succes in NVS!");
        } else {
            ESP_LOGE("HTTP", "Eroare la deschiderea NVS-ului!");
        }
    }

    // Afișare results.html pentru validare 
    char resp[256];
    snprintf(resp, sizeof(resp), 
             "<html><body><h3>Conectare receptionata! Dispozitivul se va restarta!</h3>"
             "<p>SSID selectat: <b>%s</b></p>"
             "<p>Parola introdusa: <b>%s</b></p>"
             "</body></html>", ssid, pass);
             
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    vTaskDelay(pdMS_TO_TICKS(1500)); 
    esp_restart();

    return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri      = "/index.html",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

/* URI handler structure for POST /uri */
httpd_uri_t uri_post = {
    .uri      = "/results.html",
    .method   = HTTP_POST,
    .handler  = post_handler,
    .user_ctx = NULL
};

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}