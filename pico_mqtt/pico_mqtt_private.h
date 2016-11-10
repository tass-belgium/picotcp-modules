#ifndef PICO_MQTT_PRIVATE_H
#define PICO_MQTT_PRIVATE_H

#include "pico_mqtt_data_types.h"
#include "pico_mqtt_configuration.h"

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

        // message buffers
        struct pico_mqtt_list* output_queue;
        struct pico_mqtt_list* wait_queue;
        struct pico_mqtt_list* input_queue;

        //connection
        uint16_t keep_alive_time;
        struct pico_mqtt_message* will_message;
        struct pico_mqtt_data* client_id;
        struct pico_mqtt_data* username;
        struct pico_mqtt_data* password;

        // socket
        struct pico_mqtt_data* uri;
        struct pico_mqtt_data* port;

        // status
        uint8_t connect_send;
        uint8_t status;
        uint16_t connection_attempts;
        struct pico_mqtt_packet* outgoing_packet;
        struct pico_mqtt_packet* trigger_message;
        uint8_t request_complete;

        // error
        int error;
};


#define PICO_MQTT_EMPTY (struct pico_mqtt)\
	{\
        .stream = NULL,\
        .serializer = NULL,\
        .output_queue = NULL,\
        .wait_queue = NULL,\
        .input_queue = NULL,\
        .keep_alive_time = 0,\
        .will_message = NULL,\
        .client_id = NULL,\
        .username = NULL,\
        .password = NULL,\
        .uri = NULL,\
        .port = NULL,\
        .connect_send = 0,\
        .status = 0,\
        .connection_attempts = 0,\
        .outgoing_packet = NULL,\
        .trigger_message = NULL,\
        .request_complete = 0,\
        .error = 0,\
	}


struct pico_mqtt_packet
{
        uint8_t type;
        uint8_t qos;
        int32_t packet_id;
        struct pico_mqtt_message* message;
        struct pico_mqtt_data streamed;
};

#define PICO_MQTT_PACKET_EMPTY (struct pico_mqtt_packet){\
        .type = 0,\
        .qos = 0,\
        .packet_id = -1,\
        .message = NULL,\
        .streamed = PICO_MQTT_DATA_EMPTY\
        }

/**
* Debug Functions
**/

void pico_mqtt_print_data(const struct pico_mqtt_data* data);

#endif
