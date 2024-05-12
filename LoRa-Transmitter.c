#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

#include "driver/gpio.h"
#include "../lib/lora.c"
#include "../lib/DHT11.c"

static char* create_json(int temperature, int humidity)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature", temperature);
    cJSON_AddNumberToObject(root, "humidity", humidity);
    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string;
}

void lora_task_tx(void *p)
{
   DHT11_init(GPIO_NUM_26);
    int temperature, humidity, len_json;
    char *json_string;

   temperature = DHT11_read().temperature;
   humidity = DHT11_read().humidity;
   json_string = create_json(temperature, humidity);
   len_json = strlen(json_string);
   ESP_LOGI("JSON", "%s", json_string);
   ESP_LOGI("JSON", "%d", len_json);

   while(1)
   {
      DHT11_init(GPIO_NUM_26);
      struct dht11_reading dht_data = DHT11_read();

      char *json_data = create_json(dht_data.temperature, dht_data.humidity);

      // Get the length of the JSON data
      int json_data_length = strlen(json_data);

      vTaskDelay(5000/portTICK_PERIOD_MS);

      // Send the JSON data via LoRa
      lora_send_packet((uint8_t *)json_data, json_data_length);

      ESP_LOGI("CHECKING TRANSMIT DATA", "Data sent successfully");
      ESP_LOGI("CHECKING TRANSMIT DATA", "Data sent: %s", json_data);
      printf("Lenght Data: %d\n", json_data_length);

      // Free the JSON data
      free(json_data);
   }
}


void app_main(void)
{
    lora_init();
    lora_set_frequency(433e6);
    lora_set_bandwidth(125e3);  
    lora_enable_crc();

    xTaskCreate(lora_task_tx, "lora_task_tx", 2048, NULL, 5, NULL);
}