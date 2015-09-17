#ifndef PICO_MQTT_H
#define PICO_MQTT_H

#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>

/**
* Data Structures
**/

struct pico_mqtt_data
{
	uint32_t length;
	void * data;
};

struct pico_mqtt_message
{
	uint8_t header;
	uint16_t message_id;
	struct pico_mqtt_data topic;
	struct pico_mqtt_data data;
};

#include "pico_mqtt_stream.h"
#include "pico_mqtt_list.h"
#include "pico_mqtt_serializer.h"

/**
* Program settings
**/

#define WILDCARDS 0
#define SYSTEM_TOPICS 0
#define AUTO_RECONNECT 1

#define EMPTY_CLIENT_ID 0
#define LONG_CLIENT_ID 0
#define NON_ALPHA_NUMERIC_CLIENT_ID 0

#define MAXIMUM_MESSAGE_SIZE 500
#define MAXIMUM_TOPIC_SUBSCRIPTIONS 10
#define MAXIMUM_ACTIVE_MESSAGES 10
// the memory used as a buffer, not for storing internal state or sockets
#define MAXIMUM_MEMORY_USE 2048

/**
* Public Function Prototypes
**/

// allocate memory for the pico_mqtt object and freeing it
int pico_mqtt_create(struct pico_mqtt** mqtt, const char* uri, uint32_t* timeout);
int pico_mqtt_destory(struct pico_mqtt** mqtt);

// connect to the server and disconnect again
int pico_mqtt_connect(struct pico_mqtt* mqtt, uint32_t* timeout);
int pico_mqtt_disconnect(struct pico_mqtt* mqtt);

// ping the server and flush buffer and subscribtions (back to state after create)
int pico_mqtt_restart(struct pico_mqtt* mqtt, uint32_t* timeout);
int pico_mqtt_ping(struct pico_mqtt* mqtt, uint32_t* timeout );

// receive a message or publis a message
int pico_mqtt_receive(struct pico_mqtt* mqtt, uint32_t* timeout);
int pico_mqtt_publish(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, uint32_t* timeout);

//subscribe to a topic (or multiple topics) and unsubscribe
int pico_mqtt_subscribe(struct pico_mqtt* mqtt, const char* topic, const uint8_t quality_of_service, uint32_t* timeout); //return errors, qos is provided in message upon receive
int pico_mqtt_unsubscribe(struct pico_mqtt* mqtt, const char* topic, uint32_t* timeout);

/**
* Helper Function Prototypes
**/

// free custom data types
int pico_mqtt_destroy_message(struct pico_mqtt_message * message);
int pico_mqtt_destroy_data(struct pico_mqtt_data * data);

// create and destroy custom data types
int pico_mqtt_create_data(struct pico_mqtt_data* result, const void* data, const uint32_t length);
int pico_mqtt_create_message(struct pico_mqtt_message* result, uint8_t header, uint16_t message_id, struct pico_mqtt_data topic, struct pico_mqtt_data data);

// get the last error according to the documentation (MQTT specification)
char* pico_mqtt_get_normative_error( const struct pico_mqtt* mqtt);
uint32_t pico_mqtt_get_error_documentation_line( const struct pico_mqtt* mqtt);

// get the protocol version
const char* pico_mqtt_get_protocol_version( void );

#endif
