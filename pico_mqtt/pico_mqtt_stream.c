#include "pico_mqtt_stream.h"
#include "pico_mqtt_error.h"

/**
* Data Types
**/

struct pico_mqtt_stream{
	struct pico_mqtt_socket* socket;

	struct pico_mqtt_data output_message; /* store the original pointer and length */
	struct pico_mqtt_data output_message_buffer; /* store the index and length remaining to send */
	struct pico_mqtt_data input_message; /* store the original pointer and length */
	struct pico_mqtt_data input_message_buffer; /* store the index and length remaining to receive */

	uint8_t* fixed_header_next_byte;
	uint8_t fixed_header[5];
};

/**
* Private Function Prototypes
**/

static int8_t is_time_left(struct timeval* time_left);
static int send_message(struct pico_mqtt* mqtt, struct timeval* time_left);
static int receive_message(struct pico_mqtt* mqtt, struct timeval* time_left);
static int check_output_message_status(struct pico_mqtt* mqtt);

/**
* Public Function Implementation
**/

int pico_mqtt_stream_create( struct pico_mqtt_stream** stream ){
#ifdef DEBUG
	if( stream == NULL )
	{
		PERROR("Invallid arguments (%p).\n", stream);
		return ERROR;
	}
#endif

	*stream = (struct pico_mqtt_stream*) malloc(sizeof(struct pico_mqtt_stream));
	if(*stream == NULL)
	{
		PERROR("Error while allocating the stream object.\n");
		return ERROR;
	}
	
	**stream = (struct pico_mqtt_stream)
	{
		.socket = NULL,

		.output_message = PICO_MQTT_DATA_ZERO,
		.output_message_buffer = PICO_MQTT_DATA_ZERO,
		.input_message = PICO_MQTT_DATA_ZERO,
		.input_message_buffer = PICO_MQTT_DATA_ZERO,
		.fixed_header_next_byte = 0,
		.fixed_header = {0, 0, 0, 0, 0}
	};

	if(pico_mqtt_connection_create((*stream)->socket) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	return SUCCES;
}

int pico_mqtt_stream_connect( struct pico_mqtt_stream* stream, const char* uri, const char* port){
	int result = 0;

#ifdef DEBUG
	if( stream == NULL )
	{
		PERROR("Invallid arguments (%p).\n", stream);
		return ERROR;
	}
#endif

	if(pico_mqtt_connection_open(mqtt->stream->socket, uri, port) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	PINFO("Succesfully created the I/O stream.\n");
	return SUCCES;
}

int pico_mqtt_stream_is_output_message_set( struct pico_mqtt_stream* stream, uint8_t* result)
{
#ifdef DEBUG
	if( stream == NULL )
	{
		PERROR("Invallid arguments (%p).\n", stream);
		return ERROR;
	}
#endif

	if(stream->output_message.data == NULL)
	{
		*result = 0;
	} else
	{
		*result = 1;
	}

	return SUCCES;
}

int pico_mqtt_stream_is_input_message_set( struct pico_mqtt_stream* stream, uint8_t* result ){
#ifdef DEBUG
	if( stream == NULL )
	{
		PERROR("Invallid arguments (%p).\n", stream);
		return ERROR;
	}
#endif

	/* check if the pointer is set and if the message is completely received */
	if((stream->input_message.data == NULL) || (stream->input_message_buffer.length != 0))
	{
		*result = 0;
	} else
	{
		*result = 1;
	}

	return SUCCES;
}

int pico_mqtt_stream_set_output_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data message){
	uint8_t flag = 0;

#ifdef DEBUG
	if( (stream == NULL) || (message.data == NULL) )
	{
		PERROR("Invallid arguments (%p, %p).\n", stream, message.data);
		return ERROR;
	}
#endif

	if(pico_mqtt_stream_is_output_message_set(mqtt, &flag) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(flag == 1)
	{
		PERROR("Unable to set new output message, previous output message is not yet send.\n");
		return ERROR;
	}

	stream->output_message = message;
	stream->output_message_buffer = message;

#ifdef DEBUG

#if DEBUG > 2
	PTODO("write specialized funciton to do debug printing of the message.\n");
	PINFO("The message being send is %d bytes long:\n", message.length);
#ifdef ENABLE_INFO
	pico_mqtt_print_data(message);
#endif

#endif /* DEBUG > 2 */

#endif /* defined DEBUG */

	return SUCCES;
}

int pico_mqtt_stream_get_input_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data* message){
	uint8_t flag = 0;

#ifdef DEBUG
	if( (stream == NULL) || (message == NULL) )
	{
		PERROR("Invallid arguments (%p, %p).\n", stream, message);
		return ERROR;
	}
#endif

	if(pico_mqtt_stream_is_input_message_set(mqtt, &flag) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(flag == 0)
	{
		PERROR("No message is completely received yet.\n");
		return ERROR;
	}

	*message = stream->input_message;
	stream->input_message = PICO_MQTT_DATA_ZERO;
	stream->input_message_buffer = PICO_MQTT_DATA_ZERO;
	stream->fixed_header_next_byte = 0;

	return SUCCES;
}

int pico_mqtt_stream_send_receive( struct pico_mqtt_stream* stream, int* time_left){
	uint8_t input_message_flag = 0;
	uint8_t output_message_flag = 0;
	int previous_time = get_current_time();

#ifdef DEBUG
	if( (stream == NULL)  )
	{
		PERROR("Invallid arguments (%p).\n", stream);
		return ERROR;
	}
#endif

	if(pico_mqtt_stream_is_output_message_set(stream, &output_message_flag) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(pico_mqtt_stream_is_input_message_set(mqtt, &input_message_flag) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	while(time_left != 0)
	{
		if((input_message_flag == 0))
		{
			if((output_message_flag == 0)) /* only recieve a message, no output message specified*/
			{
				if(receive_message(stream, time_left) == ERROR)
				{
					PTRACE;
					return ERROR;
				}

				if(pico_mqtt_stream_is_input_message_set(stream, &input_message_flag) == ERROR)
				{
					PTRACE;
					return ERROR;
				}
			} else /* send and receive a message */
			{
				if(send_and_receive_message(stream, time_left) == ERROR)
				{
					PTRACE;
					return ERROR;
				}

				if(pico_mqtt_stream_is_output_message_set(mqtt, &output_message_flag) == ERROR)
				{
					PTRACE;
					return ERROR;
				}

				if(pico_mqtt_stream_is_input_message_set(stream, &input_message_flag) == ERROR)
				{
					PTRACE;
					return ERROR;
				}
			}

		} else
		{
			if((output_message_flag == 0)) /* nothing more to be done */
			{
				PINFO("Succesfully send/received message.\n");
				return SUCCES;

			} else /* send a message */
			{
				if(send_message(stream, time_left) == ERROR)
				{
					PTRACE;
					return ERROR;
				}

				if(pico_mqtt_stream_is_output_message_set(mqtt, &output_message_flag) == ERROR)
				{
					PTRACE;
					return ERROR;
				}
			}
		}

		update_time_left(&previous_time, time_left);
	}
	
	PINFO("A timeout occured when sending/receiving message.\n");
	return SUCCES;
}

int pico_mqtt_stream_destroy( struct pico_mqtt_stream** stream_ptr )
{
#ifdef DEBUG
	if( (stream_ptr == NULL)  )
	{
		PERROR("Invallid arguments (%p).\n", stream_ptr);
		return ERROR;
	}
#endif

	if(pico_mqtt_connection_close(&((*stream)->socket)) == ERROR)
	{
		PTRACE;
		free(*stream_ptr);
		return ERROR;
	}
	
	free(*stream_ptr);
	*stream_ptr == NULL;

	return SUCCES;
}


/**
* Private Function Implementation
**/

static int send_receive_message(struct pico_mqtt_stream* stream, int time_left)
{
	int input_ready_flag = 0;

#ifdef DEBUG
	if(stream == NULL)
	{
		PERROR("Invallid arguments (%p).\n", stream);
		return ERROR;
	}
#endif

	if(pico_mqtt_stream_is_input_message_set(stream, &input_ready_flag) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	/* Read the fixed header */
	if(stream->input_message_buffer.length == 0)
	{
		if(input_ready_flag ==  0)
		{
			struct pico_mqtt_data header = (struct pico_mqtt_data)
			{
				.data = &stream->fixed_header[stream->fixed_header_next_byte],
				.length = 1
			};

			if(pico_mqtt_connection_send_receive(stream->socket, &header, &stream->output_message_buffer, time_left))
			{
				PTRACE;
				return ERROR;
			}

			stream->fixed_header_next_byte++;

			if(stream->fixed_header_next_byte > 2)
			{
				int length_ready = 0;
				struct pico_mqtt_data length = (struct pico_mqtt_data)
				{
					.data = &stream->fixed_header[stream->fixed_header_next_byte],
					.length = stream->fixed_header_next_byte - 1
				};

				if(pico_mqtt_is_header_complete(&length, &length_ready) == ERROR)
				{
					PTRACE;
					return ERROR;
				}

				if(length_ready == 1)
				{
					uint32_t length_converted = 0;
					int i = 0;

					if(pico_mqtt_decode_length(&length, &length_converted) == ERROR)
					{
						PTRACE;
						return ERROR;
					}

					stream->input_message_buffer.length = length_converted + stream->fixed_header_next_byte;

					if(stream->input_message_buffer.length > MAXIMUM_MESSAGE_SIZE)
					{
						PERROR("Message received is longer than maximum size allowed by MAXIMUM_MESSAGE_SIZE.\n");
						return ERROR;
					}

					PTODO("Consider checking if their is enough memory to receive the message.\n");

					stream->input_message_buffer.data = malloc( stream->input_message_buffer.length);

					if(stream->input_message_buffer.data == NULL)
					{
						PERROR("Unable to allocate memory.\n");
						return ERROR;
					}

					stream->input_message = stream->input_message_buffer;

					for(i = 0; i < stream->fixed_header_next_byte; i++)
					{
						stream->input_message_buffer.data = stream->fixed_header[i];
						++stream->input_message_buffer.data;
						--stream->input_message_buffer.length;
					}

					stream->fixed_header_next_byte = 0;
					stream->fixed_header = [0,0,0,0,0];
				}
			}
		} else {
			struct pico_mqtt_data header = PICO_MQTT_DATA_ZERO;

			if(pico_mqtt_connection_send_receive(stream->socket, &header, &stream->output_message_buffer, time_left))
			{
				PTRACE;
				return ERROR;
			}			
		}

	} else {
		if(pico_mqtt_connection_send_receive(stream->socket, &stream->input_message_buffer, &stream->output_message_buffer, time_left))
		{
			PTRACE;
			return ERROR;
		}
	}

	/* Check if an output message was set and was done sending */
	if((stream->output_message_buffer.length == 0) && (stream->output_message_buffer.data != NULL))
	{
		stream->output_message_buffer = PICO_MQTT_DATA_ZERO;

#ifdef PICO_MQTT_STREAM_FREE_DATA_WHEN_SEND
	#if PICO_MQTT_STREAM_FREE_DATA_WHEN_SEND == 1
			free(stream->output_message.data);
	#endif *//* PICO_MQTT_STREAM_FREE_DATA_WHEN_SEND == 1 *//*
#endif *//* defined PICO_MQTT_STREAM_FREE_DATA_WHEN_SEND *//*

		stream->output_message = PICO_MQTT_DATA_ZERO;

	}

	return SUCCES;
}

static void update_time_left(int* previous_time, int* time_left){
	int now = get_current_time();

	if(*time_left < 0)
	{
		*previous_time = now;
		return;
	}

	if(*time_left == 0)
	{
		*previous_time = now;
		return;
	}

	*time_left = *time_left - (now - *previous_time);
	*previous_time = now;
	if(*time_left < 0)
	{
		*time_left = 0;
	}
}

#ifdef UNUSED			
static int try_interpret_fixed_header(struct pico_mqtt* mqtt, struct pico_mqtt_data* fixed_header){
	struct pico_mqtt_stream* stream = mqtt->stream;
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
		result = mqtt->malloc(mqtt, &(stream->input_message_buffer.data), length);
		if(result != 0)
			return -2;
		stream->input_message->header = stream->fixed_header[0];
		stream->input_message_buffer.length = length;
		stream->fixed_header_next_byte = NULL;
		stream->input_message_status = 1;
		return 1;
	}
	
	return 0;
}
#endif