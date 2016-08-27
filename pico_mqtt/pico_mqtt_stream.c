#include "pico_mqtt_stream.h"

/**
* Data Types
**/

struct pico_mqtt_stream{
	struct pico_mqtt_socket* socket;

	struct pico_mqtt_data send_message; /* store the original pointer and length */
	struct pico_mqtt_data send_message_buffer; /* store the index and length remaining to send */
	struct pico_mqtt_data receive_message; /* store the original pointer and length */
	struct pico_mqtt_data receive_message_buffer; /* store the index and length remaining to receive */

	uint8_t fixed_header_next_byte;
	uint8_t fixed_header[5];
	int* error;
};

#define PICO_MQTT_STREAM_EMPTY (struct pico_mqtt_stream)\
{\
	.socket = NULL,\
	.send_message = PICO_MQTT_DATA_EMPTY,\
	.send_message_buffer = PICO_MQTT_DATA_EMPTY,\
	.receive_message = PICO_MQTT_DATA_EMPTY,\
	.receive_message_buffer = PICO_MQTT_DATA_EMPTY,\
	.fixed_header_next_byte = 0,\
	.fixed_header = {0, 0, 0, 0, 0},\
	.error = NULL\
}

/**
* Private Function Prototypes
**/

static void update_time_left(uint64_t* previous_time, uint64_t* time_left);
static int send_receive_message(struct pico_mqtt_stream* stream, uint64_t time_left);
static uint8_t receive_in_progress( const struct pico_mqtt_stream* stream);
static uint8_t is_fixed_header_complete( const struct pico_mqtt_stream* stream);
static uint8_t receiving_fixed_header( const struct pico_mqtt_stream* stream );
static uint8_t receiving_payload( const struct pico_mqtt_stream* stream );
static int process_fixed_header( struct pico_mqtt_stream* stream );
static uint8_t is_send_receive_done( struct pico_mqtt_stream* stream, uint8_t wait_for_input);

/**
* Public Function Implementation
**/

struct pico_mqtt_stream* pico_mqtt_stream_create( int* error ){
	struct pico_mqtt_stream* stream = NULL;
	CHECK_NOT_NULL( error );

	stream = (struct pico_mqtt_stream*) MALLOC(sizeof(struct pico_mqtt_stream));
	if(stream == NULL)
		return NULL;

	*stream = PICO_MQTT_STREAM_EMPTY;
	stream->error = error;

	stream->socket = pico_mqtt_connection_create( error );
	if(stream->socket == NULL)
	{
		FREE(stream);
		return NULL;
	}

	return stream;
}

int pico_mqtt_stream_connect( struct pico_mqtt_stream* stream, const char* uri, const char* port){
	CHECK_NOT_NULL(stream);

	if(pico_mqtt_connection_open(stream->socket, uri, port) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	PINFO("Succesfully created the I/O stream.\n");
	return SUCCES;
}

uint8_t pico_mqtt_stream_is_message_sending( const struct pico_mqtt_stream* stream )
{
	CHECK_NOT_NULL(stream);

	return stream->send_message_buffer.length != 0;
}

uint8_t pico_mqtt_stream_is_message_received( const struct pico_mqtt_stream* stream )
{
	CHECK_NOT_NULL(stream);

	/* check if the pointer is set and if the message is completely received */
	return (stream->receive_message.data != NULL) && (stream->receive_message_buffer.length == 0);
}

void pico_mqtt_stream_set_send_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data message)
{
	CHECK_NOT_NULL(stream);
	CHECK_NOT_NULL(message.data);
	CHECK(!pico_mqtt_stream_is_message_sending(stream),
		"Unable to set new output message, previous output message is not yet send.\n");

	stream->send_message = message;
	stream->send_message_buffer = message;

}

struct pico_mqtt_data pico_mqtt_stream_get_received_message( struct pico_mqtt_stream* stream )
{
	struct pico_mqtt_data message = PICO_MQTT_DATA_EMPTY;
	CHECK_NOT_NULL(stream);
	CHECK(pico_mqtt_stream_is_message_received(stream),
		"No message is completely received yet.\n");


	message = stream->receive_message;
	stream->receive_message = PICO_MQTT_DATA_EMPTY;
	stream->receive_message_buffer = PICO_MQTT_DATA_EMPTY;

	return message;
}

int pico_mqtt_stream_send_receive( struct pico_mqtt_stream* stream, uint64_t* time_left, uint8_t wait_for_input){
	uint64_t previous_time = get_current_time();
	CHECK_NOT_NULL(stream);

	while(*time_left != 0)
	{
		if(is_send_receive_done( stream, wait_for_input))
			return SUCCES;

		if(send_receive_message(stream, *time_left) == ERROR)
		{
			PTRACE();
			return ERROR;
		}

		update_time_left(&previous_time, time_left);
	}

	PINFO("A timeout occured while sending/receiving message.\n");
	return SUCCES;
}

void pico_mqtt_stream_destroy( struct pico_mqtt_stream* stream )
{
	if(stream == NULL)
		return;

	FREE(stream->receive_message.data);

	pico_mqtt_connection_destroy(stream->socket);
	FREE(stream);
}


/**
* Private Function Implementation
**/

static int send_receive_message(struct pico_mqtt_stream* stream, uint64_t time_left)
{
	CHECK_NOT_NULL(stream);

	if(pico_mqtt_stream_is_message_received( stream ))
	{
		if(pico_mqtt_connection_send_receive(
			stream->socket,
			&stream->send_message_buffer,
			NULL,
			time_left
		) == ERROR)
		{
			PTRACE();
			return ERROR;
		}

		return SUCCES;
	}

	if(!receiving_payload( stream ))
	{
		struct pico_mqtt_data header = (struct pico_mqtt_data)
		{
			.data = &stream->fixed_header[stream->fixed_header_next_byte],
			.length = 1
		};

		if(pico_mqtt_connection_send_receive(
			stream->socket,
			&stream->send_message_buffer,
			&header,
			time_left
		) == ERROR)
		{
			PTRACE();
			return ERROR;
		}

		if(header.length == 0)
			(stream->fixed_header_next_byte)++;

		if(process_fixed_header( stream ) == ERROR)
		{
			PTRACE();
			return ERROR;
		}

		return SUCCES;
	}

	if(pico_mqtt_connection_send_receive(
		stream->socket,
		&stream->send_message_buffer,
		&stream->receive_message_buffer,
		time_left
	) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	return SUCCES;
}

static void update_time_left(uint64_t* previous_time, uint64_t* time_left)
{
	uint64_t now = get_current_time();
	CHECK(now >= *previous_time, "Time should not go backwards.\n");

	if( *time_left > (now - *previous_time))
	{
		*time_left = *time_left - (now - *previous_time);

	} else{
		*time_left = 0;
	}

	*previous_time = now;
}

static uint8_t receive_in_progress( const struct pico_mqtt_stream* stream)
{
	CHECK_NOT_NULL(stream);

	return receiving_fixed_header( stream )
		|| receiving_payload( stream );
}

static uint8_t receiving_fixed_header( const struct pico_mqtt_stream* stream )
{
	CHECK_NOT_NULL(stream);
	return stream->fixed_header_next_byte != 0;
}

static uint8_t receiving_payload( const struct pico_mqtt_stream* stream )
{
	CHECK_NOT_NULL(stream);
	return stream->receive_message_buffer.length != 0;
}

static uint8_t is_fixed_header_complete( const struct pico_mqtt_stream* stream)
{
	uint8_t i = 1;
	CHECK_NOT_NULL(stream);

	if(stream->fixed_header_next_byte <= 1)
		return 0;

	if(stream->fixed_header_next_byte > 4)
		return 1;

	for(i = 1; i < stream->fixed_header_next_byte; ++i)
	{
		if((stream->fixed_header[i] & 0x80) == 0)
			return 1;
	}

	return 0;
}

static int process_fixed_header( struct pico_mqtt_stream* stream )
{
	uint32_t length = 0;
	uint8_t i = 0;
	CHECK_NOT_NULL( stream );

	if(!is_fixed_header_complete( stream ))
		return SUCCES;

	if(pico_mqtt_deserialize_length(stream->error, (stream->fixed_header)+1, &length) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	stream->receive_message_buffer.length = length + stream->fixed_header_next_byte;

	if(stream->receive_message_buffer.length > MAXIMUM_MESSAGE_SIZE)
	{
		PERROR("Message received is longer than maximum size allowed by MAXIMUM_MESSAGE_SIZE.\n");
		PTODO("Set the appropriate error.");
		return ERROR;
	}

	PTODO("Consider checking if their is enough memory to receive the message.\n");

	stream->receive_message_buffer.data = MALLOC( stream->receive_message_buffer.length );

	if(stream->receive_message_buffer.data == NULL)
	{
		stream->receive_message_buffer.length = 0;
		PERROR("Unable to allocate memory.\n");
		PTODO("Set the appropriate error.");
		return ERROR;
	}

	stream->receive_message = stream->receive_message_buffer;

	for(i = 0; i < stream->fixed_header_next_byte; i++)
	{
		*(uint8_t*)(stream->receive_message_buffer.data) = stream->fixed_header[i];
		stream->receive_message_buffer.data = (uint8_t*) stream->receive_message_buffer.data + 1;
		--stream->receive_message_buffer.length;
	}

	stream->fixed_header_next_byte = 0;

	return SUCCES;
}

static uint8_t is_send_receive_done( struct pico_mqtt_stream* stream, uint8_t wait_for_input)
{
	if(pico_mqtt_stream_is_message_sending(stream))
		return 0;

	if(pico_mqtt_stream_is_message_received(stream))
		return 1;

	if(!wait_for_input && !receive_in_progress( stream ))
		return 1;

	return 0;
}