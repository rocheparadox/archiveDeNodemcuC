//
// Created by Brazen Paradox on 30/4/20.
//

/*
 * This program creates a json file in flash memory if one does not exist.
 * Inside json, It looks for the key "count" and if it does not exist, then creates one.
 * Reads the value of count and prints it.
 * It then increments the count value by one and updates the count value in json with incremented value
 * The json is then written to the file and stored to the flash memory
 *
 * */

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "cJSON.h"


static const char *TAG = "ROM data storage RW";


void app_main(){
    //initialize the variables

    FILE* json_file;
    char json_string[64];

    //Initialize NVS
    //The NVS module allows you to manage non-volatile memory.
    // Using the NVS APIs, you can read and write data to persistent storage.

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //Initialize spiffs, the filesystem for ESP8266
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    // Check if our json file exists
    struct stat st;
    if (!stat("/spiffs/counter.json", &st) == 0) {
        //create if it does not exist
        // "w" option corresponds to write mode. It creates the file if it does not exist
        ESP_LOGI(TAG, "counter.json does not exist. So creating it..");
        json_file = fopen("/spiffs/counter.json", "w");

        if (json_file == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
            return;
        }

        fprintf(json_file, "{\"count\":1}\n");
        fclose(json_file);
    }

    ESP_LOGI(TAG, "Read json file");
    json_file = fopen("/spiffs/counter.json", "r");

    if (json_file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    fgets(json_string, sizeof(json_string), json_file);
    ESP_LOGI(TAG, json_string);
    fclose(json_file);

    cJSON *json =  cJSON_Parse(json_string);

    ESP_LOGI(TAG, "JSON file has value so moving ahead");
    ESP_LOGI(TAG, "data is %s", json_string);
    int count = cJSON_GetObjectItem(json, "count")->valueint;
    ESP_LOGI(TAG, "count is %d", count);

    cJSON_SetIntValue(cJSON_GetObjectItem(json, "count"), count + 1);
    ESP_LOGI(TAG, "updated json is %s", cJSON_PrintUnformatted(json));

    json_file = fopen("/spiffs/counter.json", "w");
    fprintf(json_file, cJSON_PrintUnformatted(json));
    fclose(json_file);


}