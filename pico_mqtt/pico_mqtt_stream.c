#include "pico_mqtt_stream.h"

/**
* Data Types
**/

struct pico_mqtt_stream{
	struct pico_mqtt* mqtt;
	int (*malloc)(struct pico_mqtt*, void**, size_t);
	int (*free)(struct pico_mqtt*, void*, size_t);
	struct pico_mqtt_socket* socket;

	struct pico_mqtt_data* output_message;
	struct pico_mqtt_data output_message_buffer;
	struct pico_mqtt_data* input_message;
	uint8_t input_message_status; /* 0: receive fixed header, 1: receive message, 2: ready */
	uint8_t input_message_type;
	struct pico_mqtt_data input_message_buffer;
	uint8_t* fixed_header_next_byte;
	uint8_t fixed_header[5];
};

/** Error numbers
*
*  0: No Error
*  -1: Invalid function input
*  -2: Unable to allocate memory
*  -3: TCP connection failed
*  -4: Output message already set
*  -5: Input message already set
*  -6: Incomming message to long
*
**/

/**
* Static Function Prototypes
**/

static int8_t is_time_left(struct timeval* time_left);
static int try_interpret_fixed_header(struct pico_mqtt_stream* stream, struct pico_mqtt_data* fixed_header);

/**
* Public Function Implementation
**/

int pico_mqtt_stream_create( struct pico_mqtt* mqtt, struct pico_mqtt_stream** stream, int (*my_malloc)(struct pico_mqtt*, void**, size_t), int (*my_free)(struct pico_mqtt*, void*, size_t)){
	struct pico_mqtt_stream* result;

	if( stream == NULL || my_malloc == NULL || my_free == NULL )
		return -1;

	result = (struct pico_mqtt_stream*) malloc(sizeof(struct pico_mqtt_stream));
	if(result == NULL)
		return -2;

	*result = ( struct pico_mqtt_stream ) {
		.mqtt = mqtt,
		.malloc = my_malloc,
		.free = my_free,
		.socket = mqtt->socket
	};

	*stream = result;
	return 0;
}

int pico_mqtt_stream_is_output_message_set( struct pico_mqtt_stream* stream ){
	if(stream == NULL)
		return 0;

	if(stream->output_message == NULL)
		return 0;

	return 1;
}

int pico_mqtt_stream_is_input_message_set( struct pico_mqtt_stream* stream ){
	if(stream == NULL)
		return 0;

	if(stream->input_message == NULL)
		return 0;

	return 1;
}

int pico_mqtt_stream_set_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data* message){
	if(stream == NULL || message == NULL)
		return -1;

	if(pico_mqtt_stream_is_output_message_set(stream))
		return -4;

	stream->output_message = message;
	stream->output_message_buffer = *message;
	return 0;
}

void pico_mqtt_stream_destroy( struct pico_mqtt_stream* stream ){
	free((void*) stream);
}

int pico_mqtt_stream_get_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data** message_ptr){
	if(stream == NULL || message_ptr == NULL)
		return -1;

	*message_ptr = stream->input_message;
	stream->input_message = NULL; 
	return 0;
}

int pico_mqtt_stream_send_receive( struct pico_mqtt_stream* stream, struct timeval* time_left){
	if(stream == NULL)
		return -1;

	if(pico_mqtt_stream_is_input_message_set(stream))
		return -5;

	int result;
	while(is_time_left(time_left) == 1){
		if(stream->input_message_status == 0){ /*read fixed header*/
			struct pico_mqtt_data fixed_header = {.length = 1, .data = (void*) (stream->fixed_header_next_byte)};
			result = pico_mqtt_connection_send_receive(
				stream->socket, 
				&(stream->output_message_buffer), 
				&fixed_header, 
				time_left);
			if(result == 1){ /*returned succesfull*/
				result = try_interpret_fixed_header(stream, &fixed_header);
				if(result != 1)
					return result;
			}
		} else { /*do read and write*/
			result = pico_mqtt_connection_send_receive(
				stream->socket, 
				&(stream->output_message_buffer), 
				&(stream->input_message_buffer), 
				time_left);
			if(result != 1)
				return result;
		}

	}

	return 0;
}

/**
* Private Function Implementation
**/

static int8_t is_time_left(struct timeval* time_left){
	if(time_left == NULL)
		return -1;

	return time_left->tv_sec != 0 || time_left->tv_usec != 0;
}
			
static int try_interpret_fixed_header(struct pico_mqtt_stream* stream, struct pico_mqtt_data* fixed_header){
	if(fixed_header->length == 0){ /* check if bytes where red */
		++(stream->fixed_header_next_byte);
		uint8_t bytes_red = stream->fixed_header_next_byte - &(stream->fixed_header[0]);
		uint8_t* last_red = stream->fixed_header_next_byte -1;
		uint32_t length = 0;
		int i = 1;
		int result;

		if(bytes_red == 1)
			return 0; /* take no action and wait */
		if(bytes_red == 5 && (*last_red) & 0x80)
			return -2; /* normative error, fixed header to long*/
		if((*last_red) & 0x80)
			return 0; /* not yet complete, no action */

		for(i=i; i<bytes_red; ++i){
			result |= (stream->fixed_header[i]) << (7*(i-1));
		}
#ifdef PICO_MQTT_MAXIMUM_MESSAGE_SIZE
		if(result>PICO_MQTT_MAXIMUM_MESSAGE_SIZE)
			return -6; /* message to big */
#endif
		result = stream->malloc(stream->mqtt, &(stream->input_message_buffer.data), length);
		if(result != 0)
			return -2;
		stream->input_message_type = stream->fixed_header[0];
		stream->input_message_buffer.length = length;
		stream->fixed_header_next_byte = NULL;
		stream->input_message_status = 1;
		return 1;
	}
	
	return 0;
}
