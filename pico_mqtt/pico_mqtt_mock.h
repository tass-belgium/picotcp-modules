#ifndef PICO_MQTT_MOCK_H
#define PICO_MQTT_MOCK_H

#include <stdint.h>
#include "pico_mqtt.h"

void mock_reset( void );
void mock_set_create_fail( uint8_t result );
void mock_set_return_data( struct pico_mqtt_data data );
void mock_set_length( uint32_t length );
void mock_set_error_value( int error_value );
void mock_set_return_value( int return_value );

#endif