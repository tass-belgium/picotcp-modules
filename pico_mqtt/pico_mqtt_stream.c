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
	int* error;
};

/**
* Private Function Prototypes
**/

static void update_time_left(int* previous_time, int* time_left);
static int send_receive_message(struct pico_mqtt_stream* stream, int time_left);

/**
* Public Function Implementation
**/

struct pico_mqtt_stream* pico_mqtt_stream_create( struct pico_mqtt* mqtt ){
	struct pico_mqtt_stream* stream = NULL;
	struct pico_mqtt_socket* connection = NULL;

#ifdef DEBUG
	if( mqtt == NULL )
	{
		PERROR("Invallid arguments (%p).\n", mqtt);
		EXIT_ERROR();
	}
#endif

	connection = pico_mqtt_connection_create( mqtt );
	if(connection == NULL)
	{
		PTRACE;
		return NULL;
	}

	stream = (struct pico_mqtt_stream*) malloc(sizeof(struct pico_mqtt_stream));
	if(stream == NULL)
	{
		PERROR("Error while allocating the stream object.\n");
		return NULL;
	}
	
	*stream = (struct pico_mqtt_stream)
	{
		.socket = connection,

		.output_message = PICO_MQTT_DATA_ZERO,
		.output_message_buffer = PICO_MQTT_DATA_ZERO,
		.input_message = PICO_MQTT_DATA_ZERO,
		.input_message_buffer = PICO_MQTT_DATA_ZERO,
		.fixed_header_next_byte = 0,
		.fixed_header = {0, 0, 0, 0, 0},

		.error = &mqtt->error
	};

	return stream;
}

int pico_mqtt_stream_connect( struct pico_mqtt_stream* stream, const char* uri, const char* port){
	int result = 0;

#ifdef DEBUG
	if( stream == NULL )
	{
		PERROR("Invallid arguments (%p).\n", stream);
		EXIT_ERROR();
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

uint8_t pico_mqtt_stream_is_output_message_set( struct pico_mqtt_stream* stream )
{
#ifdef DEBUG
	if( stream == NULL )
	{
		PERROR("Invallid arguments (%p).\n", stream);
		EXIT_ERROR();
	}
#endif

	return stream->output_message.data != NULL;
}

uint8_t pico_mqtt_stream_is_input_message_set( struct pico_mqtt_stream* stream ){
#ifdef DEBUG
	if( stream == NULL )
	{
		PERROR("Invallid arguments (%p).\n", stream);
		EXIT_ERROR();
	}
#endif

	/* check if the pointer is set and if the message is completely received */
	return (stream->input_message.data != NULL) && (stream->input_message_buffer.length == 0);
}

void pico_mqtt_stream_set_output_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data message)
{
#ifdef DEBUG
	if( (stream == NULL) || (message.data == NULL) )
	{
		PERROR("Invallid arguments (%p, %p).\n", stream, message.data);
		EXIT_ERROR();
	}

	if(pico_mqtt_stream_is_output_message_set(mqtt))
	{
		PERROR("Unable to set new output message, previous output message is not yet send.\n");
		EXIT_ERROR();
	}
#endif

	stream->output_message = message;
	stream->output_message_buffer = message;

#ifdef DEBUG

#ifdef ENABLE_INFO
	PINFO("The message being send is %d bytes long:\n", message.length);
	pico_mqtt_print_data(message);
#endif /* defined ENABLE_INFO */

#endif /* defined DEBUG */

	return;
}

struct pico_mqtt_data pico_mqtt_stream_get_input_message( struct pico_mqtt_stream* stream )
{
	struct pico_mqtt_data message = PICO_MQTT_DATA_ZERO;

#ifdef DEBUG
	if( (stream == NULL) || (message == NULL) )
	{
		PERROR("Invallid arguments (%p, %p).\n", stream, message);
		EXIT_ERROR();
	}

	if(pico_mqtt_stream_is_input_message_set(mqtt))
	{
		PERROR("No message is completely received yet.\n");
		EXIT_ERROR();
	}
#endif

	message = stream->input_message;
	stream->input_message = PICO_MQTT_DATA_ZERO;
	stream->input_message_buffer = PICO_MQTT_DATA_ZERO;

	PTODO("check if clearing the next byte and header is not been done before.\n");

	stream->fixed_header_next_byte = 0;
	stream->fixed_header = {0,0,0,0,0};

	return message;
}

int pico_mqtt_stream_send_receive( struct pico_mqtt_stream* stream, int* time_left, uint8_t wait_for_input){
	int previous_time = get_current_time();

#ifdef DEBUG
	if( (stream == NULL)  )
	{
		PERROR("Invallid arguments (%p).\n", stream);
		EXIT_ERROR();
	}
#endif

	while(time_left != 0)
	{
		if(!pico_mqtt_stream_is_output_message_set(stream))
		{
			if(wait_for_input || input_in_progress())
			{
				if(pico_mqtt_stream_is_input_message_set(mqtt))
				{
					PINFO("Succesfully send and receive message.\n");
					return SUCCES;
				}
			} else {
				PINFO("Succesfully send message.\n");
				return SUCCES;
			}
		}
		
		if(send_and_receive_message(stream, *time_left) == ERROR)
		{
			PTRACE;
			return ERROR;
		}

		update_time_left(&previous_time, time_left);
	}
	
	PINFO("A timeout occured while sending/receiving message.\n");
	*stream->error = TIMEOUT;
	return SUCCES;
}

void pico_mqtt_stream_destroy( struct pico_mqtt_stream* stream )
{
#ifdef DEBUG
	if(stream == NULL)
	{
		PERROR("Invallid arguments (%p).\n", stream);
		EXIT_ERROR();
	}
#endif

	pico_mqtt_connection_close(stream->socket)
	free(stream);

	return;
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
		PTODO("Make sure the total bytes used is corrected!\n");
		free(stream->output_message.data);

		stream->output_message = PICO_MQTT_DATA_ZERO;

	}

	return SUCCES;
}

static void update_time_left(int* previous_time, int* time_left)
{
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