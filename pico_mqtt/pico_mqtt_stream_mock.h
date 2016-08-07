#ifndef PICO_MQTT_STREAM_MOCK_H
#define PICO_MQTT_STREAM_MOCK_H

#include "pico_mqtt_data_types.h"

void stream_mock_reset( void );
void stream_mock_set_create_fail( uint8_t result );
void stream_mock_set_return_data( struct pico_mqtt_data data );
void stream_mock_set_length( uint32_t length );
void stream_mock_set_error_value( int error_value );
void stream_mock_set_return_value( int return_value );

#endif