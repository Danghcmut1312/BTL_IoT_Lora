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

#include "../library/lora.c"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "cJSON.h"

#define TRIGGER_TEM GPIO_NUM_27
#define TRIGGER_HUM GPIO_NUM_26

void lora_task_rx(void *p)
{
    esp_rom_gpio_pad_select_gpio(TRIGGER_TEM);
    esp_rom_gpio_pad_select_gpio(TRIGGER_HUM);
    gpio_set_direction(TRIGGER_TEM, GPIO_MODE_OUTPUT);
    gpio_set_direction(TRIGGER_HUM, GPIO_MODE_OUTPUT);
    uint8_t buf[128];
    int x;
    for(;;) {
        lora_receive();   
        while(lora_received()) {
            x = lora_receive_packet(buf, sizeof(buf) - 1); 
            if (x > 0) {
                buf[x] = 0; 
                printf("Received: %s\n", buf);

                cJSON *root = cJSON_Parse((char *)buf);

                if (root == NULL)
                {
                    printf("JSON parse errpr\n");
                    continue;
                }

                int temperature = cJSON_GetObjectItem(root, "temperature")->valueint;
                int humidity = cJSON_GetObjectItem(root, "humidity")->valueint;
                if (temperature >= 30)
                {
                    gpio_set_level(TRIGGER_TEM, 1);
                }
                else
                {
                    gpio_set_level(TRIGGER_TEM, 0);
                }

            } else {
                printf("Receive error\n");
            }
            lora_receive();
        }
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

void app_main()
{
   lora_init();
   lora_set_frequency(433e6);
   lora_enable_crc();
   xTaskCreate(&lora_task_rx, "task_rx", 2048, NULL, 5, NULL);
}