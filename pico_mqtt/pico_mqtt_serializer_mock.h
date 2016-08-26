#ifndef PICO_MQTT_SERIALIZER_MOCK_H
#define PICO_MQTT_SERIALIZER_MOCK_H

#include "pico_mqtt_data_types.h"
#include "pico_mqtt_private.h"

void serializer_mock_reset( void );
void serializer_mock_set_create_fail( uint8_t result );
void serializer_mock_set_return_data( struct pico_mqtt_data data );
void serializer_mock_set_length( uint32_t length );
void serializer_mock_set_error_value( int error_value );
void serializer_mock_set_return_value( int return_value );
void serializer_mock_set_packet( struct pico_mqtt_packet* packet );

#endif