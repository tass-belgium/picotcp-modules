#ifndef PICO_MQTT_SOCKET_MOCK_H
#define PICO_MQTT_SOCKET_MOCK_H

#include "pico_mqtt_data_types.h"

void socket_mock_reset( void );
void socket_mock_set_create_fail( uint8_t result );
void socket_mock_set_error_value( int error_value );
void socket_mock_set_return_value( int return_value );
void socket_mock_set_bytes_to_receive( uint32_t bytes_to_receive );
void socket_mock_set_bytes_to_send( uint32_t bytes_to_send );
void socket_mock_set_time( uint64_t time );
void socket_mock_set_time_interval( uint64_t time );

#endif
