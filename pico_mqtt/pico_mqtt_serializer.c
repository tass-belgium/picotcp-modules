#include "pico_mqtt_error.h"
#include "pico_mqtt_serializer.h"

/**
* Private data types
**/

struct pico_mqtt_serializer
{
	uint8_t message_type;
	uint8_t session_present;
	uint8_t return_code;
	uint16_t packet_id;
	uint8_t quality_of_service;
	uint8_t clean_session;
	uint8_t retain;
	uint8_t duplicate;
	uint8_t will_retain;

	uint16_t keep_alive;
	uint8_t client_id_flag;
	struct pico_mqtt_data client_id;
	uint8_t will_topic_flag;
	struct pico_mqtt_data will_topic;
	uint8_t will_message_flag;
	struct pico_mqtt_data will_message;
	uint8_t username_flag;
	struct pico_mqtt_data username;
	uint8_t password_flag;
	struct pico_mqtt_data password;
	uint8_t	topic_ownership;
	struct pico_mqtt_data topic;
	uint8_t message_ownership;
	struct pico_mqtt_data message;
	
	uint8_t serialized_length_data[4];
	struct pico_mqtt_data serialized_length;
	struct pico_mqtt_data stream;
	uint8_t stream_ownership;
	void* stream_index;
	int * error;
};

/**
* Private function prototypes
**/

static int create_serializer_stream( struct pico_mqtt_serializer* serializer, const uint32_t length ); /* UT */
static void destroy_serializer_stream( struct pico_mqtt_serializer* serializer ); /* UT */
static uint32_t get_bytes_left_stream( struct pico_mqtt_serializer* serializer); /* UT */
static void add_byte_stream( struct pico_mqtt_serializer* serializer, const uint8_t data_byte); /* UT */
static uint8_t get_byte_stream( struct pico_mqtt_serializer* serializer ); /* UT */
static void add_2_byte_stream( struct pico_mqtt_serializer* serializer, const uint16_t data); /*UT */
static uint16_t get_2_byte_stream( struct pico_mqtt_serializer* serializer ); /* UT */
static void add_data_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data data); /* UT */
static struct pico_mqtt_data get_data_stream( struct pico_mqtt_serializer* serializer ); /* UT */
static void add_data_and_length_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data data); /* UT */
static struct pico_mqtt_data get_data_and_length_stream( struct pico_mqtt_serializer* serializer ); /* UT */
static void add_array_stream( struct pico_mqtt_serializer* serializer, void* array, uint32_t length); /* UT */
static void skip_length_stream( struct pico_mqtt_serializer* serializer ); /* UT */
/*static int get_length_stream( struct pico_mqtt_serializer* serializer, uint32_t* result);*/
static int serialize_acknowledge( struct pico_mqtt_serializer* serializer, const uint8_t packet_type); /* UT */
static int check_serialize_connect( struct pico_mqtt_serializer* serializer); /* UT */
static uint32_t serialize_connect_get_length( struct pico_mqtt_serializer* serializer); /* UT */
static uint8_t serialize_connect_get_flags( struct pico_mqtt_serializer* serializer); /* UT */
static int deserialize_connack(struct pico_mqtt_serializer* serializer ); /* UT */
static int deserialize_publish(struct pico_mqtt_serializer* serializer ); /* UT */
static int deserialize_acknowledge(struct pico_mqtt_serializer* serializer ); /* UT */
static int deserialize_suback(struct pico_mqtt_serializer* serializer ); /* UT */
static int serialize_subscribtion( struct pico_mqtt_serializer* serializer, uint8_t subscribe); /* UT */
static int serialize_connect( struct pico_mqtt_serializer* serializer);
static int serialize_publish( struct pico_mqtt_serializer* serializer);
static int serialize_puback( struct pico_mqtt_serializer* serializer );
static int serialize_pubrec( struct pico_mqtt_serializer* serializer );
static int serialize_pubrel( struct pico_mqtt_serializer* serializer );
static int serialize_pubcomp( struct pico_mqtt_serializer* serializer );
static int serialize_subscribe( struct pico_mqtt_serializer* serializer );
static int serialize_unsubscribe( struct pico_mqtt_serializer* serializer );
static int serialize_pingreq( struct pico_mqtt_serializer* serializer );
static int serialize_pingresp( struct pico_mqtt_serializer* serializer );
static int serialize_disconnect( struct pico_mqtt_serializer* serializer );
static int deserialize_pingreq( struct pico_mqtt_serializer* serializer );
static int deserialize_pingresp( struct pico_mqtt_serializer* serializer );

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

	CHECK_NOT_NULL( error );
	
	serializer = (struct pico_mqtt_serializer*) MALLOC(sizeof(struct pico_mqtt_serializer));
	
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
	CHECK_NOT_NULL( serializer );

	if(serializer->topic_ownership != 0)
		FREE(serializer->topic.data);

	if(serializer->message_ownership != 0)
		FREE(serializer->message.data);

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
	CHECK_NOT_NULL( serializer );

	*serializer = (struct pico_mqtt_serializer){
	.keep_alive = 0,
	.client_id = PICO_MQTT_DATA_EMPTY,
	.client_id_flag = 0,
	.will_topic = PICO_MQTT_DATA_EMPTY,
	.will_topic_flag = 0,
	.will_message = PICO_MQTT_DATA_EMPTY,
	.will_message_flag = 0,
	.username = PICO_MQTT_DATA_EMPTY,
	.username_flag = 0,
	.password = PICO_MQTT_DATA_EMPTY,
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
	CHECK_NOT_NULL( serializer );

	pico_mqtt_serializer_clear( serializer );
	FREE(serializer);
	return;
}

struct pico_mqtt_data* pico_mqtt_serialize_length( struct pico_mqtt_serializer* serializer, uint32_t length )
{
 	uint32_t mask = 0x0000007F;
	uint32_t index = 0;
	uint8_t* converted_length = serializer->serialized_length_data;
	struct pico_mqtt_data* result = &serializer->serialized_length;

	CHECK_NOT_NULL( serializer );
	CHECK( length <= 0x0FFFFFFF, "Requested length is to big for mqtt protocol.\n");

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

	CHECK_NOT_NULL( length );
	CHECK_NOT_NULL( result );

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

void pico_mqtt_serializer_set_client_id( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data client_id)
{
	CHECK_NOT_NULL(serializer);

#if ALLOW_EMPTY_CLIENT_ID == 0
	CHECK((client_id.length != 0), "The client_id cannot be 0-length.\n");
	serializer->client_id = client_id;
#else
	if(client_id == NULL)
	{
		serializer->client_id = PICO_MQTT_DATA_EMPTY;
	} else {
		if((client_id.length == 0) || (client_id.data == NULL))
		{
			CHECK_NULL(client_id.data);
			CHECK((client_id.length != 0), "The client_id has an invallid structure.\n");
			serializer->client_id = PICO_MQTT_DATA_EMPTY;
		} else {
			serializer->client_id = client_id;
		}
	}
#endif

	serializer->client_id_flag = 1;
}

void pico_mqtt_serializer_set_username( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data username)
{
	CHECK_NOT_NULL(serializer);

#if ALLOW_EMPTY_USERNAME == 0
	CHECK((username.length != 0), "The username cannot be 0-length.\n");
	serializer->username = username;
#else
	if(username == NULL)
	{
		serializer->username = PICO_MQTT_DATA_EMPTY;
	} else {
		if((username.length == 0) || (username.data == NULL))
		{
			CHECK_NULL(username.data);
			CHECK((username.length != 0), "The username has an invallid structure.\n");
			serializer->username = PICO_MQTT_DATA_EMPTY;
		} else {
			serializer->username = username;
		}
	}
#endif

	serializer->username_flag = 1;
}

void pico_mqtt_serializer_set_password( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data password)
{
	CHECK_NOT_NULL(serializer);

#if ALLOW_EMPTY_PASSWORD == 0
	CHECK((password.length != 0), "The password cannot be 0-length.\n");
	serializer->password = password;
#else
	if(password == NULL)
	{
		serializer->password = PICO_MQTT_DATA_EMPTY;
	} else {
		if((password.length == 0) || (password.data == NULL))
		{
			CHECK_NULL(password.data);
			CHECK((password.length != 0), "The password has an invallid structure.\n");
			serializer->password = PICO_MQTT_DATA_EMPTY;
		} else {
			serializer->password = password;
		}
	}
#endif

	serializer->password_flag = 1;
}

void pico_mqtt_serializer_set_will_topic( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data will_topic)
{
	CHECK_NOT_NULL(serializer);

#if ALLOW_EMPTY_TOPIC == 0
	CHECK((will_topic.length != 0), "The will_topic cannot be 0-length.\n");
	serializer->will_topic = will_topic;
#else
	if(will_topic == NULL)
	{
		serializer->will_topic = PICO_MQTT_DATA_EMPTY;
	} else {
		if((will_topic.length == 0) || (will_topic.data == NULL))
		{
			CHECK_NULL(will_topic.data);
			CHECK((will_topic.length != 0), "The will_topic has an invallid structure.\n");
			serializer->will_topic = PICO_MQTT_DATA_EMPTY;
		} else {
			serializer->will_topic = will_topic;
		}
	}
#endif

	serializer->will_topic_flag = 1;
}

void pico_mqtt_serializer_set_will_message( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data will_message)
{
	CHECK_NOT_NULL(serializer);

#if ALLOW_EMPTY_MESSAGE == 0
	CHECK((will_message.length != 0), "The will_message cannot be 0-length.\n");
	serializer->will_message = will_message;
#else
	if(will_message == NULL)
	{
		serializer->will_message = PICO_MQTT_DATA_EMPTY;
	} else {
		if((will_message.length == 0) || (will_message.data == NULL))
		{
			CHECK_NULL(will_message.data);
			CHECK((will_message.length != 0), "The will_message has an invallid structure.\n");
			serializer->will_message = PICO_MQTT_DATA_EMPTY;
		} else {
			serializer->will_message = will_message;
		}
	}
#endif

	serializer->will_message_flag = 1;
}

/**
* Packet serialization
**/

int pico_mqtt_serialize( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* message)
{
	int(* const serializers[16])(struct pico_mqtt_serializer* serializer ) = 
	{
		/* 00 - RESERVED    */ NULL,
		/* 01 - CONNECT     */ serialize_connect,
		/* 02 - CONNACK     */ NULL,
		/* 03 - PUBLISH     */ serialize_publish,
		/* 04 - PUBACK      */ serialize_puback,
		/* 05 - PUBREC      */ serialize_pubrec,
		/* 06 - PUBREL      */ serialize_pubrel,
		/* 07 - PUBCOMP     */ serialize_pubcomp,
		/* 08 - SUBSCRIBE   */ serialize_subscribe,
		/* 09 - SUBACK      */ NULL,
		/* 10 - UNSUBSCRIBE */ serialize_unsubscribe,
		/* 11 - UNSUBACK    */ NULL,
		/* 12 - PINGREQ     */ serialize_pingreq,
		/* 13 - PINGRESP    */ serialize_pingresp,
		/* 14 - DISCONNECT  */ serialize_disconnect,
		/* 15 - RESERVED    */ NULL,
	};

	CHECK_NOT_NULL(serializer);
	CHECK_NOT_NULL(message);
	CHECK( serializer->message_type < 16, "Invallid message type, outside array bounds.\n");

	PTODO("Use the message as a return.\n");
	message++;

	if(serializers[serializer->message_type] == NULL)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The message cannot be serialized.\n");
		return ERROR;
	}

	/*printf("the error: %d, message type: %d\n", serializers[serializer->message_type](serializer), serializer->message_type);*/
	return serializers[serializer->message_type](serializer);
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
		/* 04 - PUBACK      */ deserialize_acknowledge,
		/* 05 - PUBREC      */ deserialize_acknowledge,
		/* 06 - PUBREL      */ deserialize_acknowledge,
		/* 07 - PUBCOMP     */ deserialize_acknowledge,
		/* 08 - SUBSCRIBE   */ NULL,
		/* 09 - SUBACK      */ deserialize_suback,
		/* 10 - UNSUBSCRIBE */ NULL,
		/* 11 - UNSUBACK    */ deserialize_acknowledge,
		/* 12 - PINGREQ     */ deserialize_pingreq,
		/* 13 - PINGRESP    */ deserialize_pingresp,
		/* 14 - DISCONNECT  */ NULL,
		/* 15 - RESERVED    */ NULL,
	};

	CHECK_NOT_NULL(serializer);
	CHECK(message.data != NULL, "Attempting to deserialize message without data.\n");
	CHECK(message.length != 0, "Attempting to deserialize 0-length message.\n");

	pico_mqtt_serializer_clear( serializer );
	serializer->stream = message;
	serializer->stream_index = serializer->stream.data;
	PTODO("Check if you want ownership stream.\n");
	/*serializer->stream_ownership = 1;*/

	message_type = (*(uint8_t*)(message.data)) >> 4;

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
	CHECK_NOT_NULL(serializer);
	CHECK_NULL(serializer->stream.data);
	CHECK((serializer->stream.length == 0), 
		"The stream is not freed correct yet, by calling create stream a memory leak can be created.\n");

	serializer->stream.data = MALLOC(length);
	serializer->stream_ownership = 1;
	serializer->stream.length = length;
	serializer->stream_index = serializer->stream.data;

	if(serializer->stream.data == NULL){
		serializer->stream_ownership = 0;
		PTODO("set the right error.\n");
		PERROR("Unable to allocate memory.\n");
		serializer->stream.length = 0;
		return ERROR;
	}

	return SUCCES;
}

static void destroy_serializer_stream( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	if(serializer->stream_ownership != 0)
	{
		FREE(serializer->stream.data);
		serializer->stream_ownership = 0;
	}

	serializer->stream = PICO_MQTT_DATA_EMPTY;
	serializer->stream_index = NULL;
	return;
}

static uint32_t get_bytes_left_stream( struct pico_mqtt_serializer* serializer)
{
	long int length = 0;

	CHECK_NOT_NULL( serializer );
	CHECK_NOT_NULL( serializer->stream.data );
	CHECK_NOT_NULL( serializer->stream_index );
	CHECK(serializer->stream_index >= serializer->stream.data,
		"Start position cannot be before start of data.\n");
	CHECK((serializer->stream.length != 0),
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
	CHECK_NOT_NULL( serializer );
	CHECK_NOT_NULL( serializer->stream.data );
	CHECK_NOT_NULL( serializer->stream_index );
	CHECK((serializer->stream.length != 0),
		"Attempting to get the length left of a stream with an error.\n");
	CHECK((get_bytes_left_stream( serializer ) > 0),
		"Attempting to write outside the stream buffer length.\n");

	*((uint8_t*) serializer->stream_index) = data_byte;
	(serializer->stream_index)++;
	return;
}

static uint8_t get_byte_stream( struct pico_mqtt_serializer* serializer )
{
	uint8_t byte = 0;
	CHECK_NOT_NULL( serializer );
	CHECK_NOT_NULL( serializer->stream.data );
	CHECK_NOT_NULL( serializer->stream_index );
	CHECK((serializer->stream.length != 0),
		"Attempting to get the length left of a stream with an error.\n");
	CHECK((get_bytes_left_stream( serializer ) > 0),
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

	CHECK_NOT_NULL( serializer );
	CHECK((get_bytes_left_stream( serializer ) >= 2),
		"Attempting to read outside the stream buffer length.\n");

	data = (uint16_t) (data + (uint16_t) get_byte_stream(serializer));
	data = (uint16_t) (data << 8u);
	data = (uint16_t) (data + (uint16_t) get_byte_stream(serializer));
	return data;
}

static void add_data_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data data)
{
	uint32_t index = 0;

	CHECK_NOT_NULL( serializer );
	CHECK_NOT_NULL( data.data );
	CHECK((get_bytes_left_stream(serializer) >= data.length), 
		"Stream is not long enough to accept the data.\n");

	for(index = 0; index < data.length; ++index)
	{
		add_byte_stream(serializer, *(((uint8_t*) data.data) + index));
	}

	return;
}

static struct pico_mqtt_data get_data_stream( struct pico_mqtt_serializer* serializer )
{
	uint32_t index = 0;
	uint32_t bytes_left = 0;
	struct pico_mqtt_data result = PICO_MQTT_DATA_EMPTY;

	CHECK_NOT_NULL( serializer );
	bytes_left = get_bytes_left_stream(serializer);

	result.data = MALLOC(bytes_left);
	if(result.data == NULL)
	{
		PERROR("Failed to allocate the memory needed.\n");
		return PICO_MQTT_DATA_EMPTY;
	}

	result.length = bytes_left;
	for(index = 0; index < bytes_left; ++index)
	{
		*(((uint8_t*)(result.data)) + index) = get_byte_stream( serializer );
	}

	return result;
}

static void add_data_and_length_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data data)
{
	CHECK((data.length) <= 0xFFFF, 
		"Can not add data longer than 65535.\n");

	add_2_byte_stream(serializer, (uint16_t) data.length);

	if(data.length != 0)
		add_data_stream(serializer, data);

	return;
}

static struct pico_mqtt_data get_data_and_length_stream( struct pico_mqtt_serializer* serializer )
{
	uint32_t index = 0;
	uint32_t bytes_left = 0;
	uint16_t length = 0;
	struct pico_mqtt_data result = PICO_MQTT_DATA_EMPTY;
	CHECK_NOT_NULL(serializer);

	bytes_left = get_bytes_left_stream(serializer);
	if(bytes_left < 2)
	{
		PTODO("Set appropriate error.\n");
		PERROR("Message is not long enough to read a 2 byte length field.\n");
		return PICO_MQTT_DATA_EMPTY;
	}

	length = get_2_byte_stream(serializer);
	if(((int32_t)bytes_left - length - 2) < 0)
	{
		PTODO("Set appropriate error.\n");
		PERROR("Invallid length, the stream is not long enough to contain the indicated data.\n");
		return PICO_MQTT_DATA_EMPTY;
	}

	if(((int32_t)bytes_left - length - 2) == 0)
	{
		result.data = NULL;
		return PICO_MQTT_DATA_EMPTY;
	}

	result.data = MALLOC(length);
	if(result.data == NULL)
	{
		PERROR("Failed to allocate the memory needed.\n");
		return PICO_MQTT_DATA_EMPTY;
	}

	result.length = length;
	for(index = 0; index < length; ++index)
	{
		*(((uint8_t*)(result.data)) + index) = get_byte_stream( serializer );
	}

	return result;
}

static void add_array_stream( struct pico_mqtt_serializer* serializer, void* array, uint32_t length)
{
	struct pico_mqtt_data data;

	CHECK_NOT_NULL( serializer );
	CHECK_NOT_NULL( array );

	data = (struct pico_mqtt_data) {.data = array, .length = length};
	add_data_stream( serializer, data);
	return;
}

static void skip_length_stream( struct pico_mqtt_serializer* serializer )
{
	uint8_t index = 0;
	while(((get_byte_stream(serializer) & 0x80) != 0) && (index < 4))
	{
		++index;
	}
}

static int serialize_connect( struct pico_mqtt_serializer* serializer)
{
	const uint8_t fixed_header = 0x10;
	uint32_t length = 0;
	uint8_t variable_header[10] = {0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x00, 0x00, 0x00};

	CHECK_NOT_NULL(serializer);

	if(check_serialize_connect( serializer ) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	length = serialize_connect_get_length( serializer );
	variable_header[7] = serialize_connect_get_flags( serializer );

	variable_header[8] = (uint8_t) ((serializer->keep_alive & 0xFF00) >> 8);
	variable_header[9] = (uint8_t) (serializer->keep_alive & 0x00FF);

	pico_mqtt_serialize_length( serializer, length );

	if(create_serializer_stream( serializer, length + 1 + serializer->serialized_length.length) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	add_byte_stream( serializer, fixed_header );
	add_data_stream( serializer, serializer->serialized_length);
	add_array_stream( serializer, variable_header, 10);

	if(serializer->client_id_flag)
		add_data_and_length_stream( serializer, serializer->client_id);

	if(	(serializer->will_topic_flag) &&
		(serializer->will_message_flag))
	{
		add_data_and_length_stream( serializer, serializer->will_topic);
		add_data_and_length_stream( serializer, serializer->will_message);
	}

	if(serializer->username_flag)
		add_data_and_length_stream( serializer, serializer->username);

	if(serializer->password_flag)
		add_data_and_length_stream( serializer, serializer->password);

	return SUCCES;
}

/*int pico_mqtt_serializer_set_duplicate_flag( struct pico_mqtt_serializer* serializer, struct pico_mqtt_packet* packet)*/

static int serialize_publish( struct pico_mqtt_serializer* serializer)
{
	uint8_t fixed_header = 0x30;
	uint32_t length = 0;

	CHECK_NOT_NULL(serializer);
	CHECK((serializer->quality_of_service < 3), "The quality of service can be at most 2.\n");

	PTODO("Check for wildcards or system topics, check if topic has length 0");

#if ALLOW_EMPTY_TOPIC == 0
	if(serializer->topic.length == 0)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("Empty topics are not allowed.\n");
		return ERROR;
	}
#endif

#if ALLOW_EMPTY_MESSAGE == 0
	if(serializer->message.length == 0)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("Empty messages are not allowed.\n");
		return ERROR;
	}
#endif

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
	{
		PTRACE();
		return ERROR;
	}

	add_byte_stream( serializer, fixed_header );
	add_data_stream( serializer, serializer->serialized_length);
	add_data_and_length_stream( serializer, serializer->topic);

	if(serializer->quality_of_service != 0)
	{
		add_2_byte_stream( serializer, serializer->packet_id );
	}

	if((serializer->message.data != NULL) && (serializer->message.length != 0))
		add_data_stream( serializer, serializer->message);

	return SUCCES;
}

static int serialize_puback( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_acknowledge( serializer, PUBACK );
}

static int serialize_pubrec( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_acknowledge( serializer, PUBREC );
}

static int serialize_pubrel( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_acknowledge( serializer, PUBREL );
}

static int serialize_pubcomp( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_acknowledge( serializer, PUBCOMP );
}

static int serialize_subscribe( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_subscribtion( serializer, 1 );
}

static int serialize_unsubscribe( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_subscribtion( serializer, 0 );
}

static int serialize_acknowledge( struct pico_mqtt_serializer* serializer, const uint8_t packet_type)
{
	uint8_t flags = 0x00;
	uint8_t packet[4] = {0x00, 0x02, 0x00, 0x00};

	CHECK_NOT_NULL( serializer );
	CHECK(
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
	{
		PTRACE();
		return ERROR;
	}

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
	CHECK_NOT_NULL(serializer);
	CHECK((
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

	CHECK_NOT_NULL(serializer);

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

	CHECK_NOT_NULL(serializer);

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
	CHECK_NOT_NULL(serializer);

	serializer->message_type = 0x02;
	byte = get_byte_stream( serializer );

	CHECK( byte >> 4 == 0x02, "The message is not a CONNACK message as expected.\n");
	CHECK( byte == 0x20, "The CONNACK message doesn't have the correct fixed header flags set.\n");
	
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

	CHECK((byte & 0xFE) == 0x00, "The CONNACK message doesn't have the correct acknowledge flags set.\n");

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
	uint8_t byte = 0;
	CHECK_NOT_NULL(serializer);

	if(get_bytes_left_stream(serializer) < 4)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the PUBLISH message is to short to be valid.\n");
		return ERROR;
	}

	serializer->message_type = 0x03;
	byte = get_byte_stream(serializer);

	CHECK((byte >> 4 == 0x03), "The message is not a PUBLISH message as expected.\n");

	serializer->duplicate = (byte & 0x08) != 0;
	serializer->quality_of_service = (uint8_t) ((byte & 0x06) >> 1);
	serializer->retain = (byte & 0x01) != 0;

	if(serializer->quality_of_service == 3)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The quality of service cannot be 3.\n");
		return ERROR;
	}

	skip_length_stream(serializer); /* assuming the stream length is correct */

	serializer->topic = get_data_and_length_stream( serializer );

#if ALLOW_EMPTY_TOPIC == 0
	if(serializer->topic.length == 0)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The topic cannot be 0-length (ALLOW_EMPTY_TOPIC == 0).\n");
		return ERROR;
	}
#endif

	if(serializer->topic.length != 0)
		serializer->topic_ownership = 1;

	if(serializer->quality_of_service != 0)
		serializer->packet_id = get_2_byte_stream(serializer);

	serializer->message = get_data_stream( serializer );

#if ALLOW_EMPTY_MESSAGE == 0
	if(serializer->message.length == 0)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The message cannot be 0-length (ALLOW_EMPTY_MESSAGE == 0).\n");

		if(serializer->topic_ownership == 1)
		{
			FREE(serializer->topic.data);
			serializer->topic_ownership = 0;
		}
		return ERROR;
	}
#endif
	
	if(serializer->message.length != 0)
	{
		serializer->message_ownership = 1;
	}

	return SUCCES;
}

static int deserialize_acknowledge(struct pico_mqtt_serializer* serializer )
{
	uint8_t byte = 0;
	CHECK_NOT_NULL(serializer);

	serializer->message_type = 0x04;
	byte = get_byte_stream( serializer );

	CHECK(
		(byte >> 4 == 0x04) ||
		(byte >> 4 == 0x05) ||
		(byte >> 4 == 0x06) ||
		(byte >> 4 == 0x07) ||
		(byte >> 4 == 0x0B),
		"The message is not an acknowledge message as expected.\n");

	CHECK( 
		(byte == 0x40) ||
		(byte == 0x50) ||
		(byte == 0x62) ||
		(byte == 0x70) ||
		(byte == 0xB0),
		"The acknowledge message doesn't have the correct fixed header flags set.\n");
	
	if(serializer->stream.length != 4)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the acknowledge message is not correct.\n");
		return ERROR;
	}

	byte = get_byte_stream( serializer );

	if(byte != 2)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the acknowledge message is not correct.\n");
		return ERROR;
	}

	serializer->packet_id = get_2_byte_stream( serializer );

	return SUCCES;
}

static int deserialize_suback(struct pico_mqtt_serializer* serializer )
{
	uint8_t byte = 0;
	CHECK_NOT_NULL(serializer);

	serializer->message_type = 0x09;
	byte = get_byte_stream( serializer );

	CHECK( byte >> 4 == 0x09, "The message is not a SUBACK message as expected.\n");
	CHECK( byte == 0x90, "The SUBACK message doesn't have the correct fixed header flags set.\n");
	
	if(serializer->stream.length != 5)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the SUBACK message is not correct.\n");
		return ERROR;
	}

	byte = get_byte_stream( serializer );

	if( byte != 3)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the SUBACK message is not correct.\n");
		return ERROR;
	}

	serializer->packet_id = get_2_byte_stream( serializer );

	byte = get_byte_stream( serializer );

	if( byte == 0x80 )
	{
		PTODO("Set appropriate error.\n");
		PERROR("The SUBACK indicated a failure.\n");
		return ERROR;
	}

	if( byte > 2)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The return value is not valid.\n");
		return ERROR;
	}

	serializer->quality_of_service = byte;

	return SUCCES;
}

static int serialize_subscribtion( struct pico_mqtt_serializer* serializer, uint8_t subscribe)
{
	CHECK_NOT_NULL(serializer);

#if ALLOW_EMPTY_TOPIC == 0
	if(serializer->topic.length == 0)
	{
		CHECK_NULL(serializer->topic.data);
		PTODO("Set appropriate error.\n");
		PERROR("The topic should be set.\n");
		return ERROR;
	}
#endif

	pico_mqtt_serialize_length( serializer, serializer->topic.length + 5 );

	if(create_serializer_stream( serializer, serializer->topic.length + 6 + serializer->serialized_length.length) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	if(subscribe != 0)
	{
		add_byte_stream( serializer, 0x82 );
	} else 
	{
		add_byte_stream( serializer, 0xA2 );
	}

	add_data_stream( serializer, serializer->serialized_length );
	add_2_byte_stream( serializer, serializer->packet_id);
	add_data_and_length_stream( serializer, serializer->topic );
	add_byte_stream( serializer, serializer->quality_of_service );

	return SUCCES;
}

static int serialize_pingreq( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);
	if(create_serializer_stream(serializer, 2) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	add_2_byte_stream( serializer, 0xC000);

	return SUCCES;
}

static int serialize_pingresp( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);
	if(create_serializer_stream(serializer, 2) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	add_2_byte_stream( serializer, 0xD000);

	return SUCCES;
}

static int serialize_disconnect( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);
	if(create_serializer_stream(serializer, 2) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	add_2_byte_stream( serializer, 0xE000);

	return SUCCES;
}

static int deserialize_pingreq( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);
	serializer->message_type = 0x0C;

	if(serializer->stream.length != 2)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The PINGREQ packet should be 2 byte long, not %d.\n", serializer->stream.length);
		return ERROR;
	}

	if(get_2_byte_stream(serializer) != 0xC000)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The PINGREQ packet is not formed correctly.\n");
		return ERROR;
	}

	return SUCCES;
}

static int deserialize_pingresp( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);
	serializer->message_type = 0x0D;

	if(serializer->stream.length != 2)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The PINGRESP packet should be 2 byte long, not %d.\n", serializer->stream.length);
		return ERROR;
	}

	if(get_2_byte_stream(serializer) != 0xD000)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The PINGRESP packet is not formed correctly.\n");
		return ERROR;
	}

	return SUCCES;
}