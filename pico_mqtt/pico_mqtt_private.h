#ifndef PICO_MQTT_PRIVATE_H
#define PICO_MQTT_PRIVATE_H

#include "pico_mqtt.h"

/**
* Data Types
**/

struct pico_mqtt
{
        // stream
        struct pico_mqtt_stream* stream;

        // serializer
        struct pico_mqtt_serializer* serializer;
        uint16_t next_message_id;

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1
        // message buffers
        struct pico_mqtt_message* trigger_message;
        struct pico_mqtt_list* output_queue;
        struct pico_mqtt_list* active_messages;
        struct pico_mqtt_list* completed_messages;
#endif /* ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1 */

        // connection related
        uint16_t keep_alive_time;
        uint8_t retain;
        uint8_t quality_of_service;
        struct pico_mqtt_data* client_id;
        struct pico_mqtt_data* will_topic;
        struct pico_mqtt_data* will_message;
        struct pico_mqtt_data* username;
        struct pico_mqtt_data* password;

        // memory
        int (*malloc)(struct pico_mqtt*, void**, size_t);
        int (*free)(struct pico_mqtt*, void*, size_t);
        uint32_t bytes_used;

        // status
        uint8_t connected;
        char normative_error[15];
        uint32_t documentation_reference;
};

/**
* Debug Functions
**/

void pico_mqtt_print_data(const struct pico_mqtt_data* data);

#endif
