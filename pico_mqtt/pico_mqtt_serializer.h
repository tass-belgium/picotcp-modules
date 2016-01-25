#ifndef PICO_MQTT_SERIALIZER_H
#define PICO_MQTT_SERIALIZER_H

#include <stdint.h>
#include "pico_mqtt.h"
#include "pico_mqtt_private.h"

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

int pico_mqtt_serialize( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* messagae); /* NO UT */
int pico_mqtt_deserialize( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data message); /* NO UT */

#endif