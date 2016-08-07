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

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1
        // message buffers
        struct pico_mqtt_list* output_queue;
        struct pico_mqtt_list* wait_queue;
        struct pico_mqtt_list* input_queue;
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
/*        int (*malloc)(struct pico_mqtt*, void**, size_t);
        int (*free)(struct pico_mqtt*, void*, size_t);*/
        uint32_t bytes_used;

        // status
        uint8_t connected;
        uint32_t connection_attempts;
        uint8_t trigger_on_receive;
        struct pico_mqtt_message* active_output_message;
        struct pico_mqtt_message* trigger_message;

        // error
        int error;
        char normative_error[15];
        uint32_t documentation_reference;
};

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1
#define PICO_MQTT_EMPTY (struct pico_mqtt)\
	{\
		.stream = NULL,\
		.serializer = NULL,\
		.next_message_id = 0,\
		.output_queue = NULL,\
		.wait_queue = NULL,\
		.input_queue = NULL,\
		.keep_alive_time = 0,\
		.retain = 0,\
		.quality_of_service = 0,\
		.client_id = NULL,\
		.will_topic= NULL,\
		.will_message = NULL,\
		.username = NULL,\
		.password = NULL,\
		.connected = 0,\
		.connection_attempts = 0,\
		.trigger_on_receive = 0,\
		.active_output_message = NULL,\
		.trigger_message = NULL,\
		.error = NO_ERROR,\
		.normative_error = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},\
		.documentation_reference = 0,\
	}
#else
#define PICO_MQTT_EMPTY (struct pico_mqtt)\
	{\
		.stream = NULL,\
		.serializer = NULL,\
		.next_message_id = 0,\
		.keep_alive_time = 0,\
		.retain = 0,\
		.quality_of_service = 0,\
		.client_id = NULL,\
		.will_topic= NULL,\
		.will_message = NULL,\
		.username = NULL,\
		.password = NULL,\
		.connected = 0,\
		.connection_attempts = 0,\
		.trigger_on_receive = 0,\
		.active_output_message = NULL,\
		.trigger_message = NULL,\
		.error = NO_ERROR,\
		.normative_error = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},\
		.documentation_reference = 0,\
	}
#endif //ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1


struct pico_mqtt_packet
{
        uint8_t type;
        uint8_t duplicate;
        uint8_t retain;
        uint8_t quality_of_service;
        uint8_t status;
        uint16_t packet_id;
        struct pico_mqtt_data topic;
        struct pico_mqtt_data data;
        struct pico_mqtt_data streamed;
};

#define PICO_MQTT_PACKET_EMPTY (struct pico_mqtt_packet){\
        .type = 0,\
        .duplicate = 0,\
        .retain = 0,\
        .quality_of_service = 0,\
        .status = 0,\
        .packet_id = 0,\
        .topic = PICO_MQTT_DATA_EMPTY,\
        .data = PICO_MQTT_DATA_EMPTY,\
        .streamed = PICO_MQTT_DATA_EMPTY\
        }

/**
* Debug Functions
**/

void pico_mqtt_print_data(const struct pico_mqtt_data* data);

#endif
