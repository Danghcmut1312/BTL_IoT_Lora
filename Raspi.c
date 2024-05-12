#include "LoRa.h"
#include "mosquitto.h"
#include <iostream>
#include <cstring>

#define MAX_DATA_SIZE 256
int counter = 0;
struct mosquitto *client;

// MQTT broker details
const char *broker_address = "demo.thingsboard.io";
const char *client_id = "Hao";
const char *password = "Hao";
const char *topic = "v1/devices/me/telemetry";
const int broker_port = 1883;
char payload[MAX_DATA_SIZE] = {0};


void on_connect(struct mosquitto *client, void *userdata, int rc)
{
    int result;
    if (rc == 0)
    {
        std::cout << "Connected to the broker" << std::endl;

        result = mosquitto_publish(client, nullptr, topic, strlen(payload), payload, 0, false);
        if (result == MOSQ_ERR_SUCCESS)
        {
            std::cout << "Data published successfully" << std::endl;
        }
        else
        {
            std::cerr << "Failed to publish data: " << mosquitto_strerror(result) << std::endl;
        }
    }
    else {
        std::cerr << "Failed to connect to the broker: " << mosquitto_strerror(rc) << std::endl;
    }
}


void setup() {  

    printf("LoRa Receiver\n");
    LoRa.setPins(10, 6, 24);

    if (!LoRa.begin(433E6, 0)) {
    printf("Starting LoRa failed!\n");
    while (1);
    }

    printf("Init LoRa Done !!\n");

    // Initialize Mosquitto library
    mosquitto_lib_init();

    client = mosquitto_new(client_id, true, nullptr);

    if (!client)
    {
        std::cerr << "Failed to create Mosquitto client" << std::endl;
        return;
    }

    mosquitto_username_pw_set(client, client_id, password);

    mosquitto_connect_callback_set(client, on_connect);

    if (mosquitto_connect(client, broker_address, broker_port, 60) != MOSQ_ERR_SUCCESS) {
        std::cerr << "Failed to connect to the broker" << std::endl;
        return ;
    }


}

void loop() {
// try to parse packet
    int packetSize = LoRa.parsePacket();

    if (packetSize) {
    // received a packet
        printf("Received packet '");

        int i = 0;

    // read packet
    while (LoRa.available()) {
        payload[i] = (char)LoRa.read();
        printf("%c", payload[i]);
        i++; 
    }

    // print RSSI of packet
        int result = mosquitto_publish(client, NULL, topic, strlen(payload), payload, 0, false);
        if (result == MOSQ_ERR_SUCCESS)
        {
            std::cout << "\nData published successfully" << std::endl;
        }
        else
        {
            std::cerr << "\nFailed to publish data: " << mosquitto_strerror(result) << std::endl;
	    //printf("\n%d", result);
            if (result == MOSQ_ERR_ERRNO) {
                // Connection was lost, attempt to reconnect
                result = mosquitto_reconnect(client);
		//printf("\n%d", result);
                if (result != MOSQ_ERR_SUCCESS) {
                    std::cerr << "Failed to reconnect: " << mosquitto_strerror(result) << std::endl;
                }
            }
	}
    }
}


int  main(void) {
   setup();
   while(1) loop();
   mosquitto_destroy(client);
   mosquitto_lib_cleanup();
}