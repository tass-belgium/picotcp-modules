#include "pico_mqtt_error.h"
#include "pico_mqtt_serializer.h"

/**
* Private data types
**/

/**
* Private function prototypes
**/

static int create_serializer_stream( struct pico_mqtt_serializer* serializer, const uint32_t length );
static void destroy_serializer_stream( struct pico_mqtt_serializer* serializer );
static void add_byte_stream( struct pico_mqtt_serializer* serializer, const uint8_t data_byte);
static void add_2_byte_stream( struct pico_mqtt_serializer* serializer, const uint16_t data);
static void add_data_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data const* data);
static void add_data_and_length_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data const* data);
static void add_array_stream( struct pico_mqtt_serializer* serializer, void* array, uint32_t length);
static int serialize_acknowledge( struct pico_mqtt_serializer* serializer, const uint8_t packet_type);
static int check_serialize_connect( struct pico_mqtt_serializer* serializer);
static uint32_t serialize_connect_get_length( struct pico_mqtt_serializer* serializer);
static uint8_t serialize_connect_get_flags( struct pico_mqtt_serializer* serializer);

static int deserialize_connack(struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data* message);

static uint32_t get_bytes_left_stream( struct pico_mqtt_serializer* serializer);

/**
* Fixed header flags
**/

#define CONNECT (1<<4)
#define CONNACK (2<<4)
#define PUBLISH (3<<4)
#define PUBACK (4<<4)
#define PUBREC (5<<4)
#define PUBREL (6<<4)
#define PUBCOMP (7<<4)
#define SUBSCRIBE (8<<4)
#define SUBACK (9<<4)
#define UNSUBSCRIBE (10<<4)
#define UNSUBACK (11<<4)
#define PINGREQ (12<<4)
#define PINGRESP (13<<4)
#define DISCONNECT (14<<4)

#define DUPLICATE (1<<3)

#define QUALITY_OF_SERVICE_0 (0<<1)
#define QUALITY_OF_SERVICE_1 (1<<1)
#define QUALITY_OF_SERVICE_2 (2<<1)

#define RETAIN (1)

struct pico_mqtt_serializer* pico_mqtt_serializer_create( int* error )
{
	struct pico_mqtt_serializer* serializer = NULL;

	PICO_MQTT_CHECK_NOT_NULL( error );
	
	serializer = (struct pico_mqtt_serializer*) malloc(sizeof(struct pico_mqtt_serializer));
	
	if(serializer == NULL){
		PTODO("set the right error.\n");
		PERROR("Unable to allocate memory for the serializer.\n");
		return NULL;
	}

	pico_mqtt_serializer_total_reset( serializer );

	serializer->error = error;
	serializer->stream.data = NULL; /* to avoid freeing random data */
	return serializer;
}

void pico_mqtt_serializer_clear( struct pico_mqtt_serializer* serializer )
{
	PICO_MQTT_CHECK_NOT_NULL( serializer );

	serializer->packet_id = 0;
	/* minimum length 1 byte, even if length == 0 */
	serializer->serialized_length = (struct pico_mqtt_data){.data = serializer->serialized_length_data, .length = 1};
	serializer->serialized_length_data[0] = 0;
	serializer->serialized_length_data[1] = 0;
	serializer->serialized_length_data[2] = 0;
	serializer->serialized_length_data[3] = 0;
	serializer->message_type = 0;
	serializer->session_present = 0;
	serializer->return_code = 0;
	serializer->retain = 0;
	serializer->duplicate = 0;
	serializer->topic_ownership = 0;
	serializer->message_ownership = 0;

	destroy_serializer_stream( serializer );
	return;
}

void pico_mqtt_serializer_total_reset( struct pico_mqtt_serializer* serializer )
{
	PICO_MQTT_CHECK_NOT_NULL( serializer );

	*serializer = (struct pico_mqtt_serializer){
	.keep_alive = 0,
	.client_id = PICO_MQTT_DATA_ZERO,
	.client_id_flag = 0,
	.will_topic = PICO_MQTT_DATA_ZERO,
	.will_topic_flag = 0,
	.will_message = PICO_MQTT_DATA_ZERO,
	.will_message_flag = 0,
	.username = PICO_MQTT_DATA_ZERO,
	.username_flag = 0,
	.password = PICO_MQTT_DATA_ZERO,
	.password_flag = 0,
	.will_retain = 0,
	.packet_id = 0,
	.error = serializer->error /* pointer should not be cleared */
	};

	pico_mqtt_serializer_clear(serializer);
	return;
}

void pico_mqtt_serializer_destroy( struct pico_mqtt_serializer* serializer )
{
	PICO_MQTT_CHECK_NOT_NULL( serializer );

	destroy_serializer_stream( serializer );
	free(serializer);
	return;
}

struct pico_mqtt_data* pico_mqtt_serialize_length( struct pico_mqtt_serializer* serializer, uint32_t length )
{
 	uint32_t mask = 0x0000007F;
	uint32_t index = 0;
	uint8_t* converted_length = serializer->serialized_length_data;
	struct pico_mqtt_data* result = &serializer->serialized_length;

	PICO_MQTT_CHECK_NOT_NULL( serializer );
	PICO_MQTT_CHECK( length <= 0x0FFFFFFF, "Requested length is to big for mqtt protocol.\n");

	/* minimum length 1 byte, even if length == 0 */
	serializer->serialized_length = (struct pico_mqtt_data){.data = serializer->serialized_length_data, .length = 1};
	serializer->serialized_length_data[0] = 0;
	serializer->serialized_length_data[1] = 0;
	serializer->serialized_length_data[2] = 0;
	serializer->serialized_length_data[3] = 0;

	for(index = 0; index < 4; ++index)
	{
		converted_length[index] = (uint8_t)((length & mask) >> 7 * index);
		if(converted_length[index] != 0)
			result->length = index + 1;

		mask = mask << 7;
	}

	for(index = 0; index < result->length; ++index)
	{
		uint8_t buffer = 0x00;
		if(index != result->length - 1)
		{
			buffer = converted_length[index];
			/* set continuation bit */
			buffer |= 0x80;
		} else {
			buffer = converted_length[index];
		}
		 
		*(((uint8_t*) result->data) + index) = buffer;
	}

	return result;
}

int pico_mqtt_deserialize_length( struct pico_mqtt_serializer* serializer, void* length_void, uint32_t* result)
{
	uint8_t* length = (uint8_t*) length_void;
	uint8_t index = 0;

	PICO_MQTT_CHECK_NOT_NULL( length );
	PICO_MQTT_CHECK_NOT_NULL( result );

	*result = 0;

	for(index = 0; index < 4; ++index)
	{
		uint32_t buffer = *(length + index);
		*result += (buffer & 0x7F) << (7 * index);
		if((buffer & 0x80) != 0)
		{
			if(index == 3)
			{
				PTODO("Set appropriate error.\n");
				*(serializer->error) = 1;
				PERROR("The length is to long for the MQTT protocol.\n");
				return ERROR;
			}
		} else {
			index = 4;
		}
	}

	return SUCCES;
}

void pico_mqtt_serializer_set_client_id( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* client_id)
{
	PICO_MQTT_CHECK_NOT_NULL(serializer);

	if(client_id == NULL)
	{
		serializer->client_id = PICO_MQTT_DATA_ZERO;
	} else {
		if((client_id->length == 0) || (client_id->data == NULL))
		{
			PICO_MQTT_CHECK_NULL(client_id->data);
			PICO_MQTT_CHECK((client_id->length != 0), "The client_id has an invallid structure.\n");
			serializer->client_id = PICO_MQTT_DATA_ZERO;
		} else {
			serializer->client_id = *client_id;
		}
	}

#if ALLOW_EMPTY_CLIENT_ID == 0
	PICO_MQTT_CHECK((serializer->client_id.length != 0), "The client_id cannot be 0-length.\n");
#endif

	serializer->client_id_flag = 1;
}

void pico_mqtt_serializer_set_username( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* username)
{
	PICO_MQTT_CHECK_NOT_NULL(serializer);

	if(username == NULL)
	{
		serializer->username = PICO_MQTT_DATA_ZERO;
	} else {
		if((username->length == 0) || (username->data == NULL))
		{
			PICO_MQTT_CHECK_NULL(username->data);
			PICO_MQTT_CHECK((username->length != 0), "The username has an invallid structure.\n");
			serializer->username = PICO_MQTT_DATA_ZERO;
		} else {
			serializer->username = *username;
		}
	}

#if ALLOW_EMPTY_USERNAME == 0
	PICO_MQTT_CHECK((serializer->username.length != 0), "The username cannot be 0-length.\n");
#endif

	serializer->username_flag = 1;
}

void pico_mqtt_serializer_set_password( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* password)
{
	PICO_MQTT_CHECK_NOT_NULL(serializer);

	if(password == NULL)
	{
		serializer->password = PICO_MQTT_DATA_ZERO;
	} else {
		if((password->length == 0) || (password->data == NULL))
		{
			PICO_MQTT_CHECK_NULL(password->data);
			PICO_MQTT_CHECK((password->length != 0), "The password has an invallid structure.\n");
			serializer->password = PICO_MQTT_DATA_ZERO;
		} else {
			serializer->password = *password;
		}
	}

#if ALLOW_EMPTY_PASSWORD == 0
	PICO_MQTT_CHECK((serializer->password.length != 0), "The password cannot be 0-length.\n");
#endif

	serializer->password_flag = 1;
}

void pico_mqtt_serializer_set_will_topic( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* will_topic)
{
	PICO_MQTT_CHECK_NOT_NULL(serializer);

	if(will_topic == NULL)
	{
		serializer->will_topic = PICO_MQTT_DATA_ZERO;
	} else {
		if((will_topic->length == 0) || (will_topic->data == NULL))
		{
			PICO_MQTT_CHECK_NULL(will_topic->data);
			PICO_MQTT_CHECK((will_topic->length != 0), "The will_topic has an invallid structure.\n");
			serializer->will_topic = PICO_MQTT_DATA_ZERO;
		} else {
			serializer->will_topic = *will_topic;
		}
	}

#if ALLOW_EMPTY_TOPIC == 0
	PICO_MQTT_CHECK((serializer->will_topic.length != 0), "The will_topic cannot be 0-length.\n");
#endif

	serializer->will_topic_flag = 1;
}

void pico_mqtt_serializer_set_will_message( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* will_message)
{
	PICO_MQTT_CHECK_NOT_NULL(serializer);

	if(will_message == NULL)
	{
		serializer->will_message = PICO_MQTT_DATA_ZERO;
	} else {
		if((will_message->length == 0) || (will_message->data == NULL))
		{
			PICO_MQTT_CHECK_NULL(will_message->data);
			PICO_MQTT_CHECK((will_message->length != 0), "The will_message has an invallid structure.\n");
			serializer->will_message = PICO_MQTT_DATA_ZERO;
		} else {
			serializer->will_message = *will_message;
		}
	}

#if ALLOW_EMPTY_MESSAGE == 0
	PICO_MQTT_CHECK((serializer->will_message.length != 0), "The will_message cannot be 0-length.\n");
#endif

	serializer->will_message_flag = 1;
}

/**
* Packet serialization
**/

int pico_mqtt_serialize_connect( struct pico_mqtt_serializer* serializer)
{
	const uint8_t fixed_header = 0x10;
	uint32_t length = 0;
	uint8_t variable_header[10] = {0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x00, 0x00, 0x00};

	PICO_MQTT_CHECK_NOT_NULL(serializer);

	if(check_serialize_connect( serializer ) == ERROR)
		return ERROR;

	length = serialize_connect_get_length( serializer );
	variable_header[7] = serialize_connect_get_flags( serializer );

	variable_header[8] = (uint8_t) ((serializer->keep_alive & 0xFF00) >> 8);
	variable_header[9] = (uint8_t) (serializer->keep_alive & 0x00FF);

	pico_mqtt_serialize_length( serializer, length );

	if(create_serializer_stream( serializer, length + 1 + serializer->serialized_length.length) == ERROR)
		return ERROR;

	add_byte_stream( serializer, fixed_header );
	add_data_stream( serializer, &serializer->serialized_length);
	add_array_stream( serializer, variable_header, 10);

	if(serializer->client_id_flag)
		add_data_and_length_stream( serializer, &serializer->client_id);

	if(	(serializer->will_topic_flag) &&
		(serializer->will_message_flag))
	{
		add_data_and_length_stream( serializer, &serializer->will_topic);
		add_data_and_length_stream( serializer, &serializer->will_message);
	}

	if(serializer->username_flag)
		add_data_and_length_stream( serializer, &serializer->username);

	if(serializer->password_flag)
		add_data_and_length_stream( serializer, &serializer->password);

	return SUCCES;
}

/*int pico_mqtt_serializer_set_duplicate_flag( struct pico_mqtt_serializer* serializer, struct pico_mqtt_packet* packet)*/

int pico_mqtt_serialize_publish( struct pico_mqtt_serializer* serializer)
{
	uint8_t fixed_header = 0x30;
	uint32_t length = 0;

	PICO_MQTT_CHECK_NOT_NULL(serializer);
	PICO_MQTT_CHECK((serializer->quality_of_service < 3), "The quality of service can be at most 2.\n");

	PTODO("Check for wildcards or system topics, check if topic has length 0");

	length = serializer->topic.length + 2 + serializer->message.length;
	if(serializer->quality_of_service != QUALITY_OF_SERVICE_0)
		length += 2; /* add packet id */

	if(serializer->retain != 0)
		fixed_header |= RETAIN;

	if(serializer->quality_of_service == 0)
		fixed_header |= QUALITY_OF_SERVICE_0;

	if(serializer->quality_of_service == 1)
		fixed_header |= QUALITY_OF_SERVICE_1;

	if(serializer->quality_of_service == 2)
		fixed_header |= QUALITY_OF_SERVICE_2;

	if(serializer->duplicate != 0)
		fixed_header |= DUPLICATE;

	pico_mqtt_serialize_length( serializer, length );

	if(create_serializer_stream( serializer, length + 1 + serializer->serialized_length.length) == ERROR)
		return ERROR;

	add_byte_stream( serializer, fixed_header );
	add_data_stream( serializer, &serializer->serialized_length);
	add_data_and_length_stream( serializer, &serializer->topic);

	if(serializer->quality_of_service != 0)
	{
		add_2_byte_stream( serializer, serializer->packet_id );
	}

	if((serializer->message.data != NULL) && (serializer->message.length != 0))
		add_data_stream( serializer, &serializer->message);

	return SUCCES;
}

int pico_mqtt_serialize_puback( struct pico_mqtt_serializer* serializer )
{
	PICO_MQTT_CHECK_NOT_NULL(serializer);

	return serialize_acknowledge( serializer, PUBACK );
}

int pico_mqtt_deserialize( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data message)
{
	uint8_t message_type = 0;

	int(* const deserializers[16])(struct pico_mqtt_serializer* serializer ) = 
	{
		/* 00 - RESERVED    */ NULL,
		/* 01 - CONNECT     */ NULL,
		/* 02 - CONNACK     */ deserialize_connack,
		/* 03 - PUBLISH     */ deserialize_publish,
		/* 04 - PUBACK      */ NULL,
		/* 05 - PUBREC      */ NULL,
		/* 06 - PUBREL      */ NULL,
		/* 07 - PUBCOMP     */ NULL,
		/* 08 - SUBSCRIBE   */ NULL,
		/* 09 - SUBACK      */ NULL,
		/* 10 - UNSUBSCRIBE */ NULL,
		/* 11 - UNSUBACK    */ NULL,
		/* 12 - PINGREQ     */ NULL,
		/* 13 - PINGRESP    */ NULL,
		/* 14 - DISCONNECT  */ NULL,
		/* 15 - RESERVED    */ NULL,
	};

	PICO_MQTT_CHECK_NOT_NULL(serializer);
	PICO_MQTT_CHECK_NOT_NULL(message);
	PICO_MQTT_CHECK(message->data != NULL, "Attempting to deserialize message without data.\n");
	PICO_MQTT_CHECK(message->length != 0, "Attempting to deserialize 0-length message.\n");

	pico_mqtt_serializer_clear( serializer );
	serializer->stream = message;
	serializer->stream_index = serializer->stream.data;
	PTODO("Check if you want ownership stream.\n");
	/*serializer->stream_ownership = 1;*/

	message_type = (*(uint8_t*)(message->data)) >> 4;

	if(deserializers[message_type] == NULL)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The message cannot be deserialized.\n");
		return ERROR;
	}

	return deserializers[message_type](serializer);
}

/**
* Private functions implementation
**/
static int create_serializer_stream( struct pico_mqtt_serializer* serializer, const uint32_t length )
{
	PICO_MQTT_CHECK_NOT_NULL(serializer);
	PICO_MQTT_CHECK_NULL(serializer->stream.data);
	PICO_MQTT_CHECK((serializer->stream.length == 0), 
		"The stream is not freed correct yet, by calling create stream a memory leak can be created.\n");

	serializer->stream.data = malloc(length);
	serializer->stream_ownership = 1;
	serializer->stream.length = length;
	serializer->stream_index = serializer->stream.data;

	if(serializer->stream.data == NULL){
		PTODO("set the right error.\n");
		PERROR("Unable to allocate memory.\n");
		serializer->stream.length = 0;
		return ERROR;
	}

	return SUCCES;
}

static void destroy_serializer_stream( struct pico_mqtt_serializer* serializer )
{
	PICO_MQTT_CHECK_NOT_NULL(serializer);

	if(serializer->stream_ownership != 0)
	{
		free(serializer->stream.data);
		serializer->stream_ownership = 0;
	}

	serializer->stream = PICO_MQTT_DATA_ZERO;
	serializer->stream_index = NULL;
	return;
}

static uint32_t get_bytes_left_stream( struct pico_mqtt_serializer* serializer)
{
	long int length = 0;

	PICO_MQTT_CHECK_NOT_NULL( serializer );
	PICO_MQTT_CHECK_NOT_NULL( serializer->stream.data );
	PICO_MQTT_CHECK_NOT_NULL( serializer->stream_index );
	PICO_MQTT_CHECK(serializer->stream_index >= serializer->stream.data,
		"Start position cannot be before start of data.\n");
	PICO_MQTT_CHECK((serializer->stream.length != 0),
		"Attempting to get the length left of a stream with an error.\n");

	length = serializer->stream.length - (serializer->stream_index - serializer->stream.data);
	if(length <= 0)
	{
		return 0;
	} else
	{
		return (uint32_t) length;
	}
}

static void add_byte_stream( struct pico_mqtt_serializer* serializer, const uint8_t data_byte)
{
	PICO_MQTT_CHECK_NOT_NULL( serializer );
	PICO_MQTT_CHECK_NOT_NULL( serializer->stream.data );
	PICO_MQTT_CHECK_NOT_NULL( serializer->stream_index );
	PICO_MQTT_CHECK((serializer->stream.length != 0),
		"Attempting to get the length left of a stream with an error.\n");
	PICO_MQTT_CHECK((get_bytes_left_stream( serializer ) > 0),
		"Attempting to write outside the stream buffer length.\n");

	*((uint8_t*) serializer->stream_index) = data_byte;
	(serializer->stream_index)++;
	return;
}

static uint8_t get_byte_stream( struct pico_mqtt_serializer* serializer )
{
	uint8_t byte = 0;
	PICO_MQTT_CHECK_NOT_NULL( serializer );
	PICO_MQTT_CHECK_NOT_NULL( serializer->stream.data );
	PICO_MQTT_CHECK_NOT_NULL( serializer->stream_index );
	PICO_MQTT_CHECK((serializer->stream.length != 0),
		"Attempting to get the length left of a stream with an error.\n");
	PICO_MQTT_CHECK((get_bytes_left_stream( serializer ) > 0),
		"Attempting to read outside the stream buffer length.\n");

	byte = *((uint8_t*) serializer->stream_index);
	(serializer->stream_index)++;
	return byte;
}

static void add_2_byte_stream( struct pico_mqtt_serializer* serializer, const uint16_t data)
{
	add_byte_stream(serializer, (uint8_t)((data & 0xFF00) >> 8));
	add_byte_stream(serializer, (uint8_t)(data & 0x00FF));
	return;
}

static uint16_t get_2_byte_stream( struct pico_mqtt_serializer* serializer )
{
	uint16_t data = 0;

	PICO_MQTT_CHECK_NOT_NULL( serializer );
	PICO_MQTT_CHECK((get_bytes_left_stream( serializer ) >= 2),
		"Attempting to read outside the stream buffer length.\n");

	data += get_byte_stream(serializer);
	data <= 8;
	data += get_byte_stream(serializer);
	return data;
}

static void add_data_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data const* data)
{
	uint32_t index = 0;

	PICO_MQTT_CHECK_NOT_NULL( serializer );
	PICO_MQTT_CHECK_NOT_NULL( data );
	PICO_MQTT_CHECK_NOT_NULL( data->data );
	PICO_MQTT_CHECK((get_bytes_left_stream(serializer) >= data->length), 
		"Stream is not long enough to accept the data.\n");

	for(index = 0; index < data->length; ++index)
	{
		add_byte_stream(serializer, *(((uint8_t*) data->data) + index));
	}

	return;
}

static struct pico_mqtt_data* get_data_stream( struct pico_mqtt_serializer* serializer )
{
	uint32_t index = 0;
	uint32_t bytes_left = 0;
	struct pico_mqtt_data* result = NULL;

	PICO_MQTT_CHECK_NOT_NULL( serializer );
	bytes_left = get_bytes_left_stream(serializer);

	result = (struct pico_mqtt_data*) malloc(bytes_left);
	if(result == NULL)
	{
		PERROR("Failed to allocate the memory needed.\n");
		return NULL;
	}

	result->length = bytes_left;
	for(index = 0; index < bytes_left; ++index)
	{
		*(((uint8_t*)result->data) + index) = get_byte_stream( serializer );
	}

	return result;
}

static void add_data_and_length_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data const* data)
{
	PICO_MQTT_CHECK((data->length) <= 0xFFFF, 
		"Can not add data longer than 65535.\n");

	add_2_byte_stream(serializer, (uint16_t) data->length);

	if(data->length != 0)
		add_data_stream(serializer, data);

	return;
}

static struct pico_mqtt_data* get_data_and_length_stream( struct pico_mqtt_serializer* serializer )
{
	uint32_t index = 0;
	uint32_t bytes_left = 0;
	uint16_t length = 0;
	struct pico_mqtt_data* result = NULL;
	PICO_MQTT_CHECK_NOT_NULL(serializer);

	bytes_left = get_bytes_left_stream(serializer);
	if(bytes_left < 2)
	{
		PTODO("Set appropriate error.\n");
		PERROR("Message is not long enough to read a 2 byte length field.\n");
		return NULL;
	}

	length = get_2_byte_stream(serializer);
	if((bytes_left - length - 2) < 0)
	{
		PTODO("Set appropriate error.\n");
		PERROR("Invallid length, the stream is not long enough to contain the indicated data.\n");
		return NULL;
	}

	result = (struct pico_mqtt_data*) malloc(length);
	if(result == NULL)
	{
		PERROR("Failed to allocate the memory needed.\n");
		return NULL;
	}

	result->length = length;
	for(index = 0; index < length; ++index)
	{
		*(((uint8_t*)result->data) + index) = get_byte_stream( serializer );
	}

	return result;
}

static void add_array_stream( struct pico_mqtt_serializer* serializer, void* array, uint32_t length)
{
	struct pico_mqtt_data data;

	PICO_MQTT_CHECK_NOT_NULL( serializer );
	PICO_MQTT_CHECK_NOT_NULL( array );

	data = (struct pico_mqtt_data) {.data = array, .length = length};
	add_data_stream( serializer, &data);
	return;
}

static void skip_length_stream( struct pico_mqtt_serializer* serializer )
{
	while((get_byte_stream(serializer) & 0x80) != 0){}
}

static int get_length_stream( struct pico_mqtt_serializer* serializer, uint32_t* result)
{
	PICO_MQTT_CHECK_NOT_NULL( serializer );
	PICO_MQTT_CHECK_NOT_NULL( result );

	if(pico_mqtt_deserialize_length( serializer, serializer->stream_index, result) == ERROR)
		return ERROR;

	skip_length_stream(serializer);
	return SUCCES;
}

static int serialize_acknowledge( struct pico_mqtt_serializer* serializer, const uint8_t packet_type)
{
	uint8_t flags = 0x00;
	uint8_t packet[4] = {0x00, 0x02, 0x00, 0x00};

	PICO_MQTT_CHECK_NOT_NULL( serializer );
	PICO_MQTT_CHECK(
		(packet_type == PUBACK) || 
		(packet_type == PUBREC) ||
		(packet_type == PUBREL) ||
		(packet_type == PUBCOMP)||
		(packet_type == UNSUBACK)
		,"The requested packet type cannot be serialized as an acknowledge message.\n");

	if(packet_type != PUBREL) /* table 2.2 - Flag Bits */
	{
		flags = 0x00;
	} else
	{
		flags = 0x02;
	}

	if(create_serializer_stream( serializer, 4 ) == ERROR)
		return ERROR;

	packet[0] |= packet_type;
	packet[0] |= flags;
	PTODO("Check little and big endian.\n");
	packet[2] |= (uint8_t)((serializer->packet_id & 0xFF00) >> 8); /* MSB */
	packet[3] |= (uint8_t)(serializer->packet_id & 0x00FF); /* LSB */
	add_array_stream( serializer, packet, 4 );

	return SUCCES;
}

static int check_serialize_connect( struct pico_mqtt_serializer* serializer)
{
	PICO_MQTT_CHECK_NOT_NULL(serializer);
	PICO_MQTT_CHECK((
		serializer->quality_of_service == 0 ||
		serializer->quality_of_service == 1 ||
		serializer->quality_of_service == 2 ),
		"Will QoS not wihtin range.\n");

#if ALLOW_EMPTY_CLIENT_ID == 0
	if(serializer->client_id_flag == 0)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The client_id is not set but is obliged.\n");
		return ERROR;
	}
#endif /* ALLOW_EMPTY_CLIENT_ID == 0 */

	if(serializer->username_flag == 0)
	{
		if(serializer->password_flag != 0)
		{
			PTODO("set appropriate error.\n");
			PERROR("Password can not be set without username.\n");
			return ERROR;
		}
	}

	if(	(serializer->will_topic_flag!= 0) !=
		(serializer->will_message_flag != 0))
	{
		PTODO("set appropriate error.\n");
		PERROR("If will topic is set, will message must be set as well.\n");
		return ERROR;
	}

	return SUCCES;
}

static uint32_t serialize_connect_get_length( struct pico_mqtt_serializer* serializer)
{
	uint32_t length = 10; /* add 10 length for standard variable header */

	PICO_MQTT_CHECK_NOT_NULL(serializer);

	length += serializer->client_id.length + 2;

	if(serializer->username_flag != 0){
		length += serializer->username.length + 2;

		if(serializer->password_flag != 0)
			length += serializer->password.length + 2;
	}

	if(	(serializer->will_topic_flag != 0) &&
		(serializer->will_message_flag != 0))
	{
		length += serializer->will_topic.length + 2;
		length += serializer->will_message.length + 2;
	}

	return length;
}

static uint8_t serialize_connect_get_flags( struct pico_mqtt_serializer* serializer)
{
	uint8_t flags = 0;

	PICO_MQTT_CHECK_NOT_NULL(serializer);

	if(serializer->username_flag != 0){
		flags |= 0x80; /* User Name Flag */

		if(serializer->password_flag != 0)
			flags |= 0x40; /* password Flag */
	}

	if((serializer->clean_session != 0) ||
		(serializer->client_id_flag == 0))
		flags |= 0x02; /* Clean Session */

	if(	(serializer->will_topic_flag != 0) &&
		(serializer->will_message_flag != 0))
	{
		flags |= 0x04; /* Will Flag */
		flags |= (uint8_t)((serializer->quality_of_service & 0x03) << 3); /* Will QoS */

		if(serializer->will_retain != 0)
			flags |= 0x20; /* Will Retain */
	}

	return flags;
}

static int deserialize_connack(struct pico_mqtt_serializer* serializer )
{
	uint8_t byte = 0;
	PICO_MQTT_CHECK_NOT_NULL(serializer);
	PICO_MQTT_CHECK_NOT_NULL(serializer->stream);

	serializer->message_type = 0x02;
	byte = get_byte_stream( serializer );

	PICO_MQTT_CHECK( byte >> 4 == 0x02), "The message is not a CONNACK message as expected.\n");
	PICO_MQTT_CHECK( byte == 0x20), "The CONNACK message doesn't have the correct fixed header flags set.\n");
	
	if(serializer->stream.length != 4)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the CONNACK message is not correct.\n");
		return ERROR;
	}

	byte = get_byte_stream( serializer );

	if( byte != 2)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the CONNACK message is not correct.\n");
		return ERROR;
	}

	byte = get_byte_stream( serializer );

	PICO_MQTT_CHECK((byte & 0xFE) == 0x00, "The CONNACK message doesn't have the correct acknowledge flags set.\n");

	serializer->session_present = (byte & 0x01) == 0x01;

	serializer->return_code = get_byte_stream( serializer );

	if(serializer->return_code > 5)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The return code of the CONNACK message is unknown.\n");
		return ERROR;
	}

	return SUCCES;
}

static int deserialize_publish(struct pico_mqtt_serializer* serializer )
{
	uint8_t* message_data = NULL;
	uint8_t byte = 0;
	uint32_t length = 0;
	PICO_MQTT_CHECK_NOT_NULL(serializer);

	if(get_bytes_left_stream(serializer) < 4)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the PUBLISH message is to short to be valid.\n");
		return ERROR;
	}

	serializer->message_type = 0x03;
	byte = get_byte_stream(serializer);

	PICO_MQTT_CHECK((byte >> 4 == 0x03), "The message is not a PUBLISH message as expected.\n");

	message_data = (uint8_t*)(message->data);

	
	serializer->duplicate = (byte & 0x08) != 0;
	serializer->quality_of_service = (byte & 0x06);
	serializer->retain = (byte & 0x01) != 0;

	if(serializer->quality_of_service == 3)
	{
		PTODO("Set appropriate error.\n")
		PERROR("The quality of service cannot be 3.\n");
		return ERROR;
	}

	skip_length_stream(serializer); /* assuming the stream length is correct */

	serializer->topic = get_data_and_length_stream( serializer );
	if(serializer->topic == NULL)
		return ERROR;

	serializer->topic_ownership = 1;

	if(serializer->quality_of_service != 0)
		serializer->packet_id = get_2_byte_stream(serializer);

	serializer->message = get_data_stream( serializer );
	if(serializer->message == NULL)
		return ERROR;

	serializer->message_ownership = 1;

	return SUCCES;
}