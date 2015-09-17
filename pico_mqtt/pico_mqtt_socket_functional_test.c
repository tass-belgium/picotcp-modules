#define DEBUG 3

#include "pico_mqtt_socket.h"
#include "pico_mqtt_error.h"

int main()
{
	int result = 0;
	uint32_t written = 0;
	struct pico_mqtt_socket* socket;
	struct timeval time_left = {.tv_usec = 0, .tv_sec = 1};
	char message[12] = "Hello World\n";
	
	pico_mqtt_connection_open(&socket, "localhost", &time_left);
	pico_mqtt_connection_write(socket, message, 12, &time_left, &written);
	return SUCCES;
}
