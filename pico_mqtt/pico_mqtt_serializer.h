#ifndef PICO_MQTT_SERIALIZER_H
#define PICO_MQTT_SERIALIZER_H

#include "pico_mqtt_data_types.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_debug.h"

/**
* Function prototypes
**/

struct pico_mqtt_serializer* pico_mqtt_serializer_create( int* error );
void pico_mqtt_serializer_destroy( struct pico_mqtt_serializer* serializer );
void pico_mqtt_serializer_clear( struct pico_mqtt_serializer* serializer );

struct pico_mqtt_data* pico_mqtt_serialize_length( struct pico_mqtt_serializer* serializer, uint32_t length );
int pico_mqtt_deserialize_length( int* error, void* length_void, uint32_t* result);

void pico_mqtt_serializer_set_message_type( struct pico_mqtt_serializer* serializer, uint8_t message_type );
void pico_mqtt_serializer_set_client_id( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* client_id);
void pico_mqtt_serializer_set_username( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* username);
void pico_mqtt_serializer_set_password( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* password);
void pico_mqtt_serializer_set_message( struct pico_mqtt_serializer* serializer, struct pico_mqtt_message* message);
void pico_mqtt_serializer_set_keep_alive_time( struct pico_mqtt_serializer* serializer, uint16_t time);
void pico_mqtt_serializer_set_message_id( struct pico_mqtt_serializer* serializer, uint16_t message_id);
void pico_mqtt_serializer_set_clean_session( struct pico_mqtt_serializer* serializer, uint8_t clean_session);
void pico_mqtt_serializer_set_topic( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* topic);
void pico_mqtt_serializer_set_quality_of_service( struct pico_mqtt_serializer* serializer, uint8_t qos);
struct pico_mqtt_packet* pico_mqtt_serializer_get_packet( struct pico_mqtt_serializer* serializer);

int pico_mqtt_serialize( struct pico_mqtt_serializer* serializer); /* NO UT */
int pico_mqtt_deserialize( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data message); /* NO UT */

/**
* Message types
**/

#define CONNECT 1
#define CONNACK 2
#define PUBLISH 3
#define PUBACK 4
#define PUBREC 5
#define PUBREL 6
#define PUBCOMP 7
#define SUBSCRIBE 8
#define SUBACK 9
#define UNSUBSCRIBE 10
#define UNSUBACK 11
#define PINGREQ 12
#define PINGRESP 13
#define DISCONNECT 14

#endif
