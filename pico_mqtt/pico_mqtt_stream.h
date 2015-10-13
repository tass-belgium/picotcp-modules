#include <sys/time.h>
#include "pico_mqtt.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_socket.h"

struct pico_mqtt_stream;

int pico_mqtt_stream_create( struct pico_mqtt* mqtt );
int pico_mqtt_stream_connect( struct pico_mqtt* mqtt, const char* uri, const char* port);
int pico_mqtt_stream_is_output_message_set( struct pico_mqtt* mqtt, uint8_t* result);
int pico_mqtt_stream_is_input_message_set( struct pico_mqtt* mqtt, uint8_t* result);
int pico_mqtt_stream_set_output_message( struct pico_mqtt* mqtt, struct pico_mqtt_message* message);
int pico_mqtt_stream_get_input_message( struct pico_mqtt* mqtt, struct pico_mqtt_message** message_ptr);
int pico_mqtt_stream_send_receive( struct pico_mqtt* mqtt, struct timeval* time_left);
int pico_mqtt_stream_destroy( struct pico_mqtt* mqtt );
