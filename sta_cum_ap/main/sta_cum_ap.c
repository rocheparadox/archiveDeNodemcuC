/* Simple WiFi Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/ets_sys.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define AP_MAX_CONN	10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "station cum access_point";

int get_length(uint8_t*);

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;

    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "got ip:%s",
                     ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                     MAC2STR(event->event_info.sta_connected.mac),
                     event->event_info.sta_connected.aid);
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "station:"MACSTR" leave, AID=%d",
                     MAC2STR(event->event_info.sta_disconnected.mac),
                     event->event_info.sta_disconnected.aid);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
            if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
                /*Switch to 802.11 bgn mode */
                esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
            }
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void start_an_access_point(uint8_t* access_point_ssid, uint8_t* access_point_password)
{

    ESP_LOGI(TAG, "Gonna create an access point with creds SSID=%s, PASSWORD=%s", access_point_ssid, access_point_password);

    //Create a wifi configuration type variable to provide details for it to act as an access point
    wifi_config_t wifi_config = {
            .ap = {
                    .max_connection = AP_MAX_CONN,
                    .authmode = WIFI_AUTH_WPA_WPA2_PSK
            },
    };

    uint8_t ssid_length = get_length(access_point_ssid);
    memcpy(wifi_config.sta.ssid, access_point_ssid, get_length(access_point_ssid));
    memcpy(wifi_config.sta.password, access_point_password, get_length(access_point_password));
    wifi_config.ap.ssid_len=ssid_length;

    if (strlen((char*)access_point_password) == 0) {
        //When you do not care about people peeping to your network, do this.
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             access_point_ssid, access_point_password);
}

void connect_to_access_point(uint8_t* ssid, uint8_t* password)
{

    ESP_LOGI(TAG, "Gonna connect to the access point with creds SSID=%s, PASSWORD=%s", ssid, password);

    //Create a wifi configuration type variable to provide details of the access point to which it has to connect
    wifi_config_t wifi_config = {
            .sta ={},
    };

    memcpy(wifi_config.sta.ssid, ssid, get_length(ssid));
    memcpy(wifi_config.sta.password, password, get_length(password));

    //strcpy((char*)wifi_config.sta.ssid, (char*)ssid);
    //strcpy((char*)wifi_config.sta.password, (char*)password);

    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    //Set mode to WIFI_MODE_APSTA, so it can act as both station and access point
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             ssid, password);
}

int get_length(uint8_t *str){
    int len=0;
    //printf("\nstring is %s", str);
    while(str[len] != 0){
        //printf("\nchar is %c", str[len]);
        len++;
    }

    //return sizeof(str);
    //Add one for null termination
    return len+1;
}

void app_main()
{
    //Initialize the variables
    uint8_t ssid_to_connect[32]="reverance";
    uint8_t password4ssid_to_connect[64]="edwardsnowden";

    uint8_t access_point_ssid[32]="gra77h0pp3r";
    uint8_t access_point_password[64]="experimental001";



    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    //Pre-requisites -- initialization if you will to get the device working
    wifi_event_group = xEventGroupCreate();

    //Initialize the wifi adapter
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    /*Initialize the wifi device with the macros -- default valuess
     * Ref: https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/network/esp_wifi.html
     * search for WIFI_INIT_CONFIG_DEFAULT
    */

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /*Ref: https://github.com/esp8266/Arduino/issues/1661
     * In WIFI_MODE_APSTA, the device had to be connected to an access point before it can act as an access point
     * for it to be stable.
    */

    ESP_LOGI(TAG, "Connect to the Access point as a station ");
    connect_to_access_point(ssid_to_connect, password4ssid_to_connect);


    ESP_LOGI(TAG, "Create an access point");
    start_an_access_point(access_point_ssid, access_point_password);

}