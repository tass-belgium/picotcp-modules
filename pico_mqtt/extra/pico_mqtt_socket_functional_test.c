#define DEBUG 3

#include <stdio.h>
#include "pico_mqtt.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_socket.h"

int main()
{
	int result = 0;
	uint32_t written = 0;
	struct pico_mqtt_socket* socket;
	struct timeval time_left = {.tv_usec = 0, .tv_sec = 1};
	char message[12] = "Hello World\0";
	struct pico_mqtt_data output_buffer= {.length = 12, .data = (void*) message};
	
	
	pico_mqtt_connection_open(socket, "127.0.0.1", 1083);
	pico_mqtt_connection_send(socket, &output_buffer, &time_left);
	return SUCCES;
}
