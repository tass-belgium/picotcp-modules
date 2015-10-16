#include <sys/time.h>
#include "pico_mqtt.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_socket.h"

#define PICO_MQTT_STREAM_FREE_DATA_WHEN_SEND 1

struct pico_mqtt_stream;

/* allocate memory for the stream object */
int pico_mqtt_stream_create( struct pico_mqtt* mqtt );

/* connect to the server */
int pico_mqtt_stream_connect( struct pico_mqtt* mqtt, const char* uri, const char* port);

/* check if the output message is set */
int pico_mqtt_stream_is_output_message_set( struct pico_mqtt* mqtt, uint8_t* result);

/* check if the input message is set */
int pico_mqtt_stream_is_input_message_set( struct pico_mqtt* mqtt, uint8_t* result);

/* set the output message, data will be freed if PICO_MQTT_STREAM_FREE_DATA_WHEN_SEND is 1 */
int pico_mqtt_stream_set_output_message( struct pico_mqtt* mqtt, struct pico_mqtt_message* message);

/* get the input message if one is set */
int pico_mqtt_stream_get_input_message( struct pico_mqtt* mqtt, struct pico_mqtt_message** message_ptr);

/* start sending and receiving the messages until timeout or until done */
int pico_mqtt_stream_send_receive( struct pico_mqtt* mqtt, struct timeval* time_left);

/* free the memory allocated by the create function */
int pico_mqtt_stream_destroy( struct pico_mqtt* mqtt );
