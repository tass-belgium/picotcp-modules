#ifndef PICO_MQTT_SOCKET_H
#define PICO_MQTT_SOCKET_H

#include <netdb.h>
#include <stdint.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "pico_mqtt_data_types.h"
#include "pico_mqtt_configuration.h"
#include "pico_mqtt_debug.h"

struct pico_mqtt_socket;

/* create pico mqtt socket */
struct pico_mqtt_socket* pico_mqtt_connection_create( int* error );

/* connect to the URI*/
int pico_mqtt_connection_open(struct pico_mqtt_socket* socket, const char* uri, const char* port);

/* read data from the socket, add this to the read buffer */
/*int pico_mqtt_connection_receive( struct pico_mqtt_socket* socket, struct pico_mqtt_data* read_buffer, struct timeval* time_left);*/

/* write data to the socket, remove this from the write buffer*/
/*int pico_mqtt_connection_send( struct pico_mqtt_socket* connection, struct pico_mqtt_data* write_buffer, struct timeval* time_left);*/

int pico_mqtt_connection_send_receive(
  struct pico_mqtt_socket* socket,
  struct pico_mqtt_data* send_buffer,
  struct pico_mqtt_data* receive_buffer,
  uint64_t time_left
);

void pico_mqtt_connection_destroy( struct pico_mqtt_socket* socket);

/* get current time in miliseconds */
uint64_t get_current_time( void  );

#endif
