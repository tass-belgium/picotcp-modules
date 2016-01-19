#include "pico_mqtt.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_socket.h"

/**
* Data structures
**/

struct pico_mqtt_stream;

/**
* Public function prototypes
**/

/* allocate memory for the stream object */
struct pico_mqtt_stream* pico_mqtt_stream_create( struct pico_mqtt* mqtt );

/* connect to the server */
int pico_mqtt_stream_connect( struct pico_mqtt_stream* stream, const char* uri, const char* port);

/* check if the output message is set */
uint8_t pico_mqtt_stream_is_output_message_set( struct pico_mqtt_stream* stream );

/* check if the input message is set */
uint8_t pico_mqtt_stream_is_input_message_set( struct pico_mqtt* mqtt );

/* set the output message, data will be freed if PICO_MQTT_STREAM_FREE_DATA_WHEN_SEND is 1 */
void pico_mqtt_stream_set_output_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data message);

/* get the input message if one is set */
struct pico_mqtt_data pico_mqtt_stream_get_input_message( struct pico_mqtt_stream* stream );

/* start sending and receiving the messages until timeout or until done */
int pico_mqtt_stream_send_receive( struct pico_mqtt_stream* stream, int* time_left, uint8_t wait_for_input);

/* free the memory allocated by the create function */
void pico_mqtt_stream_destroy( struct pico_mqtt_stream* stream );