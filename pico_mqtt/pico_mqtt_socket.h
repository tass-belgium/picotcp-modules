#ifndef PICO_MQTT_SOCKET_H
#define PICO_MQTT_SOCKET_H

#include <stdint.h>

// create pico mqtt socket and connect to the URI, returns -1 on error
int pico_mqtt_socket_open(const char* URI, uint32_t timeout);

// read data from the socket, return the number of bytes read
int pico_mqtt_socket_read( int socket_file_descriptor, void* read_buffer, uint16_t count, uint32_t timeout);

// write data to the socket, return the number of bytes written
int pico_mqtt_socket_write( int socket_file_descriptor, void* write_buffer, uint16_t count, uint32_t timeout);

// close pico mqtt socket, return -1 on error
int pico_mqtt_socket_close( int socket_file_descriptor );

#endif
