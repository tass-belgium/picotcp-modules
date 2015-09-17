#ifndef PICO_MQTT_PRIVATE_H
#define PICO_MQTT_PRIVATE_H

#include "pico_mqtt.h"

/**
* Data Types
**/

struct pico_mqtt
{
/*        struct pico_mqtt* this;*/

        // stream
        struct pico_mqtt_stream* stream;

        // serializer
        struct pico_mqtt_serializer* serializer;
        uint16_t next_message_id;

	// message buffers
	struct pico_mqtt_list* output_queue;
	struct pico_mqtt_list* active_messages;

        // connection related
        uint16_t keep_alive_time;
        struct pico_mqtt_data* client_id;
        struct pico_mqtt_data* will_topic;
        struct pico_mqtt_data* will_message;
        struct pico_mqtt_data* user_name;
        struct pico_mqtt_data* password;

        // memory
        int (*malloc)(struct pico_mqtt*, void**, size_t);
        int (*free)(struct pico_mqtt*, void*, size_t);
        uint32_t bytes_used;

        // status
        uint8_t connected;
        char normative_error[15];
        uint32_t documentation_reference;
        uint16_t next_packet_id;
};

#endif
