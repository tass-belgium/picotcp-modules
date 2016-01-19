#ifndef PICO_MQTT_SERIALIZER_H
#define PICO_MQTT_SERIALIZER_H

#include <stdint.h>
#include "pico_mqtt.h"
#include "pico_mqtt_private.h"

/**
* Data structures
**/

struct pico_mqtt_serializer
{
	uint8_t message_type;
	uint8_t session_present;
	uint8_t return_code;
	uint16_t packet_id;
	uint8_t quality_of_service;
	uint8_t clean_session;
	uint8_t retain;
	uint8_t duplicate;

	uint16_t keep_alive;
	uint8_t client_id_flag;
	struct pico_mqtt_data client_id;
	uint8_t will_topic_flag;
	struct pico_mqtt_data will_topic;
	uint8_t will_message_flag;
	struct pico_mqtt_data will_message;
	uint8_t will_retain;
	uint8_t username_flag;
	struct pico_mqtt_data username;
	uint8_t password_flag;
	struct pico_mqtt_data password;
	uint8_t	topic_ownership;
	struct pico_mqtt_data topic;
	uint8_t message_ownership;
	struct pico_mqtt_data message;
	
	uint8_t serialized_length_data[4];
	struct pico_mqtt_data serialized_length;
	struct pico_mqtt_data stream;
	uint8_t stream_ownership;
	void* stream_index;
	int * error;
};

/**
* Function prototypes
**/

struct pico_mqtt_serializer* pico_mqtt_serializer_create( int* error );
void pico_mqtt_serializer_clear( struct pico_mqtt_serializer* serializer );
void pico_mqtt_serializer_total_reset( struct pico_mqtt_serializer* serializer );
void pico_mqtt_serializer_destroy( struct pico_mqtt_serializer* serializer );
struct pico_mqtt_data* pico_mqtt_serialize_length( struct pico_mqtt_serializer* serializer, uint32_t length );
int pico_mqtt_deserialize_length( struct pico_mqtt_serializer* serializer, void* length_void, uint32_t* result);

void pico_mqtt_serializer_set_client_id( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* client_id);
void pico_mqtt_serializer_set_username( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* username);
void pico_mqtt_serializer_set_password( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* password);
void pico_mqtt_serializer_set_will_topic( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* will_topic);
void pico_mqtt_serializer_set_will_message( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* will_message);

int pico_mqtt_serialize_connect( struct pico_mqtt_serializer* serializer);
int pico_mqtt_serialize_publish( struct pico_mqtt_serializer* serializer);
int pico_mqtt_serialize_puback( struct pico_mqtt_serializer* serializer );
int pico_mqtt_deserialize( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data* message);

#endif