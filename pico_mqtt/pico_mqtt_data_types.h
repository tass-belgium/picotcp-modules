#ifndef PICO_MQTT_DATA_TYPES_H
#define PICO_MQTT_DATA_TYPES_H

#include <stdint.h>

/**
* Data Structures
**/

struct pico_mqtt_data
{
	uint32_t length;
	void * data;
};

#define PICO_MQTT_DATA_EMPTY (struct pico_mqtt_data){.data = NULL, .length = 0}

struct pico_mqtt_message
{
	uint8_t duplicate;
	uint8_t retain;
	uint8_t quality_of_service;
	uint16_t message_id;
	struct pico_mqtt_data* topic;
	struct pico_mqtt_data* data;
};

#define PICO_MQTT_MESSAGE_EMPTY (struct pico_mqtt_message){\
	.duplicate = 0,\
	.retain = 0,\
	.quality_of_service = 0,\
	.message_id = 0,\
	.topic = NULL,\
	.data = NULL\
	}

#endif /* #ifndef PICO_MQTT_DATA_TYPES_H */