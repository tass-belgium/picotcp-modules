#include "pico_mqtt_data_types.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_socket.h"
#include "pico_mqtt_serializer.h"
#include "pico_mqtt_debug.h"

/**
* Data structures
**/

struct pico_mqtt_stream;

/**
* Public function prototypes
**/

struct pico_mqtt_stream* pico_mqtt_stream_create( int* error );
int pico_mqtt_stream_connect( struct pico_mqtt_stream* stream, const char* uri, const char* port);
uint8_t pico_mqtt_stream_is_message_sending( const struct pico_mqtt_stream* stream );
uint8_t pico_mqtt_stream_is_message_received( const struct pico_mqtt_stream* stream );
void pico_mqtt_stream_set_send_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data message);
struct pico_mqtt_data pico_mqtt_stream_get_received_message( struct pico_mqtt_stream* stream );
int pico_mqtt_stream_send_receive( struct pico_mqtt_stream* stream, uint64_t* time_left, uint8_t wait_for_input);
void pico_mqtt_stream_destroy( struct pico_mqtt_stream* stream );