#include <sys/time.h>
#include "pico_mqtt.h"
#include "pico_mqtt_socket.h"

struct pico_mqtt_stream;

int pico_mqtt_stream_create( struct pico_mqtt* mqtt, struct pico_mqtt_stream** stream, int (*my_malloc)(struct pico_mqtt*, void**, size_t), int (*my_free)(struct pico_mqtt*, void*, size_t));
int pico_mqtt_stream_is_output_message_set( struct pico_mqtt_stream* stream );
int pico_mqtt_stream_is_input_message_set( struct pico_mqtt_stream* stream );
int pico_mqtt_stream_set_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data* message);
int pico_mqtt_stream_get_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data** message_ptr);
int pico_mqtt_stream_send_receive( struct pico_mqtt_stream* stream, struct timeval* time_left);
void pico_mqtt_stream_destroy( struct pico_mqtt_stream* stream );
