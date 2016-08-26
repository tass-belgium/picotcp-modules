#ifndef PICO_MQTT_LIST_MOCK_H
#define PICO_MQTT_LIST_MOCK_H

#include "pico_mqtt_data_types.h"
#include "pico_mqtt_private.h"

void list_mock_reset( void );
void list_mock_set_create_fail( uint8_t result );
void list_mock_set_return_data( struct pico_mqtt_data data );
void list_mock_set_length( uint32_t length );
void list_mock_set_error_value( int error_value );
void list_mock_set_return_value( int return_value );
void list_mock_set_length( uint32_t length );
void list_mock_set_packet( struct pico_mqtt_packet* packet );

#endif