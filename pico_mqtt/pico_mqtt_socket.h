#ifndef PICO_MQTT_SOCKET_H
#define PICO_MQTT_SOCKET_H

#include <netdb.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

/* enable dns lookup or only allow IP addresses*/
#define PICO_MQTT_DNS_LOOKUP 1

struct pico_mqtt_socket;

/* create pico mqtt socket and connect to the URI, returns NULL on error*/ 
struct pico_mqtt_socket* pico_mqtt_connection_open( const char* URI, int timeout);

/* read data from the socket, return the number of bytes read */ 
int pico_mqtt_connection_read( struct pico_mqtt_socket* socket, void* read_buffer, const uint32_t count, int timeout);

/* write data to the socket, return the number of bytes written*/ 
int pico_mqtt_connection_write( struct pico_mqtt_socket* socket, void* write_buffer, const uint32_t count, int timeout); 

int pico_mqtt_connection_send_receive( struct pico_mqtt_socket* socket, struct pico_mqtt_data* read_buffer, struct pico_mqtt_data* write_buffer, struct timeval* time_left);

/* close pico mqtt socket, return -1 on error */ 
int pico_mqtt_connection_close( struct pico_mqtt_socket* socket);

#endif
