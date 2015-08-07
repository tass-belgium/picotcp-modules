#ifndef PICO_MQTT_SOCKET_H
#define PICO_MQTT_SOCKET_H

#include <netdb.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

/* enable dns lookup or only allow IP addresses*/
#define PICO_MQTT_DNS_LOOKUP 1

struct pico_mqtt_socket;

/* create pico mqtt socket and connect to the URI, returns -1 on error */
struct pico_mqtt_socket* pico_mqtt_connection_open( const char* URI, uint32_t timeout);

/* read data from the socket, return the number of bytes read */
int pico_mqtt_connection_read( struct pico_mqtt_socket* socket, void* read_buffer, uint16_t count, uint32_t timeout);

/* write data to the socket, return the number of bytes written */
int pico_mqtt_connection_write( struct pico_mqtt_socket* socket, void* write_buffer, uint16_t count, uint32_t timeout);

/* close pico mqtt socket, return -1 on error*/
int pico_mqtt_connection_close( struct pico_mqtt_socket* socket );

#endif
