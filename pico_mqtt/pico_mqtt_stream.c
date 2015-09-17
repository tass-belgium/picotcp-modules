#include "pico_mqtt_stream.h"
#include "pico_mqtt_error.h"

/**
* Data Types
**/

struct pico_mqtt_stream{
	struct pico_mqtt_socket* socket;

	struct pico_mqtt_message* output_message;
	struct pico_mqtt_data output_message_buffer;
	struct pico_mqtt_message* input_message;
	struct pico_mqtt_data input_message_buffer;

	uint8_t input_message_status; /* 0: receive fixed header, 1: receive message, 2: ready */

	uint8_t* fixed_header_next_byte;
	uint8_t fixed_header[5];
};

/**
* Private Function Prototypes
**/

static int8_t is_time_left(struct timeval* time_left);
static int try_interpret_fixed_header(struct pico_mqtt* mqtt, struct pico_mqtt_data* fixed_header);
static int send_message(struct pico_mqtt* mqtt, uint8_t* flag, struct timeval* time_left);
static int receive_message(struct pico_mqtt* mqtt, uint8_t* flag, struct timeva* time_left);

/**
* Public Function Implementation
**/

int pico_mqtt_stream_create( struct pico_mqtt* mqtt, const char* uri, uint32_t* time_left){
	int result;
	struct timeval timeout = {.tv_usecs = 0, .tv_secs = 0};
#ifdef DEBUG
	if( mqtt == NULL )
	{
		return ERROR;
	}
#endif

	mqtt->stream = (struct pico_mqtt_stream*) malloc(sizeof(struct pico_mqtt_stream));
	if(mqtt->stream == NULL)
	{
		return ERROR;
	}
	
	*(mqtt->stream) = 
	{
		.socket = NULL,
		.output_message = NULL,
		.output_message_buffer = 
		{
			.length = 0,
			.data = NULL
		},
		.input_message = NULL,
		.input_message_buffer =
		{
			.length = 0,
			.data = NULL
		},
		input_message_status = 0,
		.fixed_header_next_byte = 0,
		.fixed_header = {0, 0, 0, 0, 0}
	};
	
	result = ms_to_struct_timeval(&timeout, time_left);
	if(result == ERROR)
	{
		return ERRROR;
	}

	result = pico_mqtt_connection_open(&(mqtt->stream->socket), uri, &timeout);
	if(result == ERROR)
	{
		return ERROR;
	}

	return SUCCES;
}

int pico_mqtt_stream_is_output_message_set( struct pico_mqtt* mqtt, uint8_t* result){
#ifdef DEBUG
	if(mqtt == NULL)
	{
		return ERROR;
	}

	if((mqtt->stream == NULL) || (result == NULL))
	{
		return ERROR;
	}
#endif

	if(mqtt->stream->output_message == NULL)
	{
		*result = 0;
	} else
	{
		*result = 1;
	}

	return SUCCES;
}

int pico_mqtt_stream_is_input_message_set( struct pico_mqtt* mqtt, uint8_t* result ){
#ifdef DEBUG
	if(mqtt == NULL)
	{
		return ERROR;
	}

	if((mqtt->stream == NULL) || (result == NULL))
	{
		return ERROR;
	}
#endif
	if(mqtt->stream->input_message == NULL)
	{
		*result = 0;
	} else
	{
		*result = 1;
	}

	return SUCCES;
}

int pico_mqtt_stream_set_message( struct pico_mqtt* mqtt, struct pico_mqtt_message* message){
	int result = 0;
	uint8_t flag = 0;

#ifdef DEBUG
	if(mqtt == NULL)
	{
		return ERROR;
	}

	if((mqtt->stream == NULL) || (message == NULL))
	{
		return ERROR;
	}
#endif

	result = pico_mqtt_stream_is_output_message_set(mqtt->stream, &flag);
	if(result == ERROR)
	{
		return ERROR;
	}

	if(flag == 1)
	{
		return ERROR;
		/* set an error that the message is set */
	}

	mqtt->stream->output_message = message;
	mqtt->stream->output_message_buffer = *(message->data);
	return SUCCES;
}

int pico_mqtt_stream_get_message( struct pico_mqtt* mqtt, struct pico_mqtt_message** message_ptr){
	int result = 0;
	uint8_t flag = 0;

#ifdef DEBUG
	if(mqtt == NULL)
	{
		return ERROR;
	}

	if((mqtt->stream == NULL) || (message_ptr == NULL))
	{
		return ERROR;
	}
#endif

	result = pico_mqtt_stream_is_input_message_set(mqtt->stream, &flag);
	if(result == ERROR)
	{
		return ERROR;
	}

	if(flag == 0)
	{
		return ERROR;
		/* set an error that their is no input message set */
	}

	*message_ptr = mqtt->stream->input_message;
	mqtt->stream->input_message = NULL; 
	return 0;
}

int pico_mqtt_stream_send_receive( struct pico_mqtt* mqtt, uint32_t* time_left){
	int result = 0;
	uint8_t input_message_flag = 0;
	uint8_t output_message_flag = 0;

#ifdef DEBUG
	if(mqtt == NULL)
	{
		return ERROR;
	}

	if(mqtt->stream == NULL)
	{
		return ERROR;
	}
#endif

	result = pico_mqtt_stream_is_output_message_set(mqtt, &output_message_flag);
	if(result == ERROR)
	{
		return ERROR;
	}

	result = pico_mqtt_stream_is_input_message_set(mqtt, &input_message_flag);
	if(result == ERROR)
	{
		return ERROR;
	}

	while((is_time_left(time_left) == 1))
	{
		if((input_message_flag == 0))
		{

			if((output_message_flag == 0)) /* only recieve a message */
			{
				result = receive_message(mqtt, &input_message_flag, time_left);
				if(result == ERROR)
				{
					return ERROR;
				}
			} else /* send and receive a message */
			{
				result = send_message(mqtt, &output_message_flag, time_left);
				if(result == ERROR)
				{
					return ERROR;
				}

				result = receive_message(mqtt, &input_message_flag, time_left);
				if(result == ERROR)
				{
					return ERROR;
				}
			}

		} else
		{
			if((output_message_flag == 0)) /* nothing more to be done */
			{
				return SUCCES;		
			} else /* send a message */
			{
				result = send_message(mqtt, &output_message_flag, time_left);
				if(result == ERROR)
				{
					return ERROR;
				}
			}
		}
	}
	
	/* timeout occurred */
	return SUCCES;
}

int pico_mqtt_stream_destroy( struct pico_mqtt* mqtt )
{
	int result = 0;
#ifdef DEBUG
	if(mqtt == NULL)
	{
		return ERROR;
	}
#endif

	result = pico_mqtt_connection_close(&(mqtt->stream->socket));
	if(result == ERROR)
	{
		free(stream);
		return ERROR;
	}
	
	free(stream);
	return SUCCES;
}


/**
* Private Function Implementation
**/

static int send_message(struct pico_mqtt* mqtt, uint8_t* flag, struct timeval* time_left)
{
	int result = 0;
#ifdef DEBUG
	if(mqtt->stream->socket == NULL)
	{
		return ERROR;
	}
#endif
	result = pico_mqtt_connection_write(mqtt->stream->socket, mqtt->stream->output_message_buffer, time_left);
	if(result == ERROR)
	{
		return ERROR;
	}

	result = check_message_status(mqtt);
	if(result == ERROR)
	{
		return ERROR;
	}

	return SUCCES;
}

static int receive_message(struct pico_mqtt* mqtt, uint8_t* flag, struct timeval* time_left);

static int check_message_status(struct pico_mqtt* mqtt)
{
	int result = 0;

	if(mqtt->stream->output_message_buffer.length == 0) /* done sending message */
	{
		mqtt->stream->output_message_buffer = {.length = 0, .data = NULL}; /* clear the buffer */
		PTODO("Use the correct free method to remove a message\n");
		/*result = mqtt->free(&(mqtt->stream->output_message));  free the message */

		if(result == ERROR)
		{
			return ERROR;
		}
	}
}

static int8_t is_time_left(struct timeval* time_left){
	if(time_left == NULL)
	{
		/* infinite time left */
		return 1;
	}

	return ((time_left->tv_sec != 0) || (time_left->tv_usec != 0));
}
			
static int try_interpret_fixed_header(struct pico_mqtt* mqtt, struct pico_mqtt_data* fixed_header){
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
