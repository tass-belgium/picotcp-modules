#include "pico_mqtt_serializer.h"

/**
* Private data types
**/

struct pico_mqtt_serializer
{
	uint8_t message_type;
	uint8_t session_present;
	uint8_t return_code;
	uint16_t message_id;
	uint8_t quality_of_service;
	uint8_t clean_session;
	uint8_t retain;
	uint8_t duplicate;

	uint16_t keep_alive;
	const struct pico_mqtt_data* client_id;
	const struct pico_mqtt_data* username;
	const struct pico_mqtt_data* password;
	struct pico_mqtt_data* topic;
	uint8_t topic_ownership;
	struct pico_mqtt_data* data;
	uint8_t data_ownership;

	uint8_t serialized_length_data[4];
	struct pico_mqtt_data serialized_length;
	struct pico_mqtt_data stream;
	uint8_t stream_ownership;
	void* stream_index;
	int * error;
};

#define PICO_MQTT_SERIALIZER_EMPTY (struct pico_mqtt_serializer){\
	.message_type = 0,\
	.session_present = 0,\
	.return_code = 0,\
	.message_id = 0,\
	.quality_of_service = 0,\
	.clean_session = 0,\
	.retain = 0,\
	.duplicate = 0,\
	.keep_alive = 0,\
	.client_id = NULL,\
	.username = NULL,\
	.password = NULL,\
	.topic = NULL,\
	.topic_ownership = 0,\
	.data = NULL,\
	.data_ownership = 0,\
	.serialized_length_data[0] = 0,\
	.serialized_length_data[1] = 0,\
	.serialized_length_data[2] = 0,\
	.serialized_length_data[3] = 0,\
	.stream = PICO_MQTT_DATA_EMPTY,\
	.stream_ownership = 0,\
	.stream_index = 0,\
	.error = NULL,\
	}

/**
* Private function prototypes
**/

static int create_stream( struct pico_mqtt_serializer* serializer, const uint32_t length );
static void destroy_serializer_stream( struct pico_mqtt_serializer* serializer );
static uint32_t get_bytes_left_stream( struct pico_mqtt_serializer* serializer);
static void add_byte_stream( struct pico_mqtt_serializer* serializer, const uint8_t data_byte);
static uint8_t get_byte_stream( struct pico_mqtt_serializer* serializer );
static void add_2_byte_stream( struct pico_mqtt_serializer* serializer, const uint16_t data);
static uint16_t get_2_byte_stream( struct pico_mqtt_serializer* serializer );
static void add_data_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data data);
static struct pico_mqtt_data* get_data_stream( struct pico_mqtt_serializer* serializer );
static void add_data_and_length_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data* data);
static struct pico_mqtt_data* get_data_and_length_stream( struct pico_mqtt_serializer* serializer );
static void add_array_stream( struct pico_mqtt_serializer* serializer, void* array, uint32_t length);
static void skip_length_stream( struct pico_mqtt_serializer* serializer );
static int serialize_acknowledge( struct pico_mqtt_serializer* serializer, const uint8_t packet_type);
static int check_serialize_connect( struct pico_mqtt_serializer* serializer);
static uint32_t serialize_connect_get_length( struct pico_mqtt_serializer* serializer);
static uint8_t serialize_connect_get_flags( struct pico_mqtt_serializer* serializer);
static int deserialize_connack(struct pico_mqtt_serializer* serializer );
static int deserialize_publish(struct pico_mqtt_serializer* serializer );
static int deserialize_acknowledge(struct pico_mqtt_serializer* serializer );
static int deserialize_suback(struct pico_mqtt_serializer* serializer );
static int serialize_subscribtion( struct pico_mqtt_serializer* serializer, uint8_t subscribe);
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

#define CONNECT_FLAG (1<<4)
#define CONNACK_FLAG (2<<4)
#define PUBLISH_FLAG (3<<4)
#define PUBACK_FLAG (4<<4)
#define PUBREC_FLAG (5<<4)
#define PUBREL_FLAG (6<<4)
#define PUBCOMP_FLAG (7<<4)
#define SUBSCRIBE_FLAG (8<<4)
#define SUBACK_FLAG (9<<4)
#define UNSUBSCRIBE_FLAG (10<<4)
#define UNSUBACK_FLAG (11<<4)
#define PINGREQ_FLAG (12<<4)
#define PINGRESP_FLAG (13<<4)
#define DISCONNECT_FLAG (14<<4)

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

	*serializer = PICO_MQTT_SERIALIZER_EMPTY;

	serializer->error = error;
	serializer->stream.data = NULL; /* to avoid freeing random data */
	return serializer;
}

void pico_mqtt_serializer_clear( struct pico_mqtt_serializer* serializer )
{
	int* error = serializer->error;
	CHECK_NOT_NULL( serializer );
	destroy_serializer_stream( serializer );

	if(serializer->data_ownership)
	{
		if(serializer->data != NULL)
			FREE(serializer->data->data);

		FREE(serializer->data);
	}

	if(serializer->topic_ownership)
	{
		if(serializer->topic != NULL)
			FREE(serializer->topic->data);

		FREE(serializer->topic);
	}

	*serializer = PICO_MQTT_SERIALIZER_EMPTY;
	serializer->error = error;
}

void pico_mqtt_serializer_destroy( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL( serializer );
	pico_mqtt_serializer_clear( serializer );
	FREE(serializer);
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

int pico_mqtt_deserialize_length( int* error, void* length_void, uint32_t* result)
{
	uint8_t* length = (uint8_t*) length_void;
	uint8_t index = 0;

	CHECK_NOT_NULL( error );
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
				*error = 1;
				PERROR("The length is to long for the MQTT protocol.\n");
				return ERROR;
			}
		} else {
			index = 4;
		}
	}

	return SUCCES;
}

void pico_mqtt_serializer_set_message_type( struct pico_mqtt_serializer* serializer, uint8_t message_type )
{
	CHECK_NOT_NULL(serializer);
	CHECK(message_type < 16, "Invallid message_type\n");

	serializer->message_type = message_type;
}

void pico_mqtt_serializer_set_client_id( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* client_id)
{
	CHECK_NOT_NULL(serializer);	

#if ALLOW_EMPTY_CLIENT_ID == 0
	CHECK_NOT_NULL(client_id);
	CHECK((client_id->length != 0) && (client_id->data != NULL),
		"Empty client id is not allowed. Check configuration.\n");
	CHECK((client_id->length == 0) == (client_id->data == NULL),
		"The client_id has an invallid structure.\n");
#endif

	serializer->client_id = client_id;
}

void pico_mqtt_serializer_set_username( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* username)
{
	CHECK_NOT_NULL(serializer);	

#if ALLOW_EMPTY_USERNAME == 0
	CHECK_NOT_NULL(username);
	CHECK((username->length != 0) && (username->data != NULL),
		"Empty username is not allowed. Check configuration.\n");
	CHECK((username->length == 0) == (username->data == NULL),
		"The username has an invallid structure.\n");
#endif

	serializer->username = username;
}

void pico_mqtt_serializer_set_password( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* password)
{
	CHECK_NOT_NULL(serializer);
	serializer->password = password;
}

void pico_mqtt_serializer_set_message( struct pico_mqtt_serializer* serializer, struct pico_mqtt_message* message)
{
	CHECK_NOT_NULL(serializer);

	if(message == NULL)
	{
		serializer->data = NULL;
		serializer->topic = NULL;
		serializer->duplicate = 0;
		serializer->retain = 0;
		serializer->quality_of_service = 0;
		serializer->message_id = 0;
		return;
	}

	CHECK((message->data->length == 0) == (message->data->data == NULL),
		"The message data has an invallid structure.\n");
	CHECK((message->topic->length == 0) == (message->topic->data == NULL),
		"The message topic has an invallid structure.\n");
	CHECK((message->topic->length != 0) && (message->topic->data != NULL),
		"The message topic must contain a valid, non zero topic.\n");

	serializer->data = message->data;
	serializer->topic = message->topic;
	serializer->duplicate = message->duplicate;
	serializer->retain = message->retain;
	serializer->quality_of_service = message->quality_of_service;
}

void pico_mqtt_serializer_set_keep_alive_time( struct pico_mqtt_serializer* serializer, uint16_t time)
{
	CHECK_NOT_NULL(serializer);
	serializer->keep_alive = time;
}

void pico_mqtt_serializer_set_message_id( struct pico_mqtt_serializer* serializer, uint16_t message_id)
{
	CHECK_NOT_NULL(serializer);
	serializer->message_id = message_id;
}

void pico_mqtt_serializer_set_clean_session( struct pico_mqtt_serializer* serializer, uint8_t clean_session)
{
	CHECK_NOT_NULL(serializer);
	CHECK(clean_session < 1, "Invallid clean session value\n");
	serializer->clean_session = clean_session;
}

void pico_mqtt_serializer_set_topic( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* topic)
{
	CHECK_NOT_NULL(serializer);

	CHECK((topic->length == 0) == (topic->data == NULL),
		"The topic has an invallid structure.\n");
	CHECK((topic->length != 0) && (topic->data != NULL),
		"The topic must contain a valid, non zero topic.\n");

	serializer->topic = topic;
}

void pico_mqtt_serializer_set_quality_of_service( struct pico_mqtt_serializer* serializer, uint8_t qos)
{
	CHECK_NOT_NULL(serializer);
	CHECK(qos<3, "invallid qos\n");

	serializer->quality_of_service = qos;
}

/**
* Packet serialization
**/

int pico_mqtt_serialize( struct pico_mqtt_serializer* serializer )
{
	int(* const serializers[16])(struct pico_mqtt_serializer* serializer ) =
	{
		NULL,					// 00 - RESERVED    
		serialize_connect,		// 01 - CONNECT_FLAG     
		NULL,					// 02 - CONNACK_FLAG     
		serialize_publish,		// 03 - PUBLISH_FLAG     
		serialize_puback,		// 04 - PUBACK_FLAG      
		serialize_pubrec,		// 05 - PUBREC_FLAG      
		serialize_pubrel,		// 06 - PUBREL_FLAG      
		serialize_pubcomp,		// 07 - PUBCOMP_FLAG     
		serialize_subscribe,	// 08 - SUBSCRIBE_FLAG	   
		NULL,					// 09 - SUBACK_FLAG      
		serialize_unsubscribe,	// 10 - UNSUBSCRIBE 
		NULL,					// 11 - UNSUBACK_FLAG    
		serialize_pingreq,		// 12 - PINGREQ_FLAG     
		serialize_pingresp,		// 13 - PINGRESP_FLAG    
		serialize_disconnect,	// 14 - DISCONNECT_FLAG  
		NULL,					// 15 - RESERVED    
	};

	CHECK_NOT_NULL(serializer);
	CHECK( serializer->message_type < 16, "Invallid message type, outside array bounds.\n");

	PTODO("Use the message as a return.\n");

	if(serializers[serializer->message_type] == NULL)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The message cannot be serialized.\n");
		return ERROR;
	}

	return serializers[serializer->message_type](serializer);
}

int pico_mqtt_deserialize( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data message)
{
	uint8_t message_type = 0;

	int(* const deserializers[16])(struct pico_mqtt_serializer* serializer ) =
	{
		/* 00 - RESERVED    */ NULL,
		/* 01 - CONNECT_FLAG     */ NULL,
		/* 02 - CONNACK_FLAG     */ deserialize_connack,
		/* 03 - PUBLISH_FLAG     */ deserialize_publish,
		/* 04 - PUBACK_FLAG      */ deserialize_acknowledge,
		/* 05 - PUBREC_FLAG      */ deserialize_acknowledge,
		/* 06 - PUBREL_FLAG      */ deserialize_acknowledge,
		/* 07 - PUBCOMP_FLAG     */ deserialize_acknowledge,
		/* 08 - SUBSCRIBE_FLAG
	   */ NULL,
		/* 09 - SUBACK_FLAG      */ deserialize_suback,
		/* 10 - UNSUBSCRIBE */ NULL,
		/* 11 - UNSUBACK_FLAG    */ deserialize_acknowledge,
		/* 12 - PINGREQ_FLAG     */ deserialize_pingreq,
		/* 13 - PINGRESP_FLAG    */ deserialize_pingresp,
		/* 14 - DISCONNECT_FLAG  */ NULL,
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

struct pico_mqtt_packet* pico_mqtt_serializer_get_packet( struct pico_mqtt_serializer* serializer)
{
	struct pico_mqtt_packet* packet = (struct pico_mqtt_packet*) MALLOC(sizeof(struct pico_mqtt_packet));
	struct pico_mqtt_message* message = (struct pico_mqtt_message*) MALLOC(sizeof(struct pico_mqtt_message));
	CHECK_NOT_NULL(serializer);

	if((packet == NULL) || (message == NULL))
	{
		PERROR("Failed to allocate the memory needed.\n");
		PTODO("set the appropriate error.\n");
		return NULL;
	}

	packet->type = serializer->message_type;
	packet->qos = serializer->quality_of_service;
	packet->packet_id = serializer->message_id;
	packet->message = message;
	message->topic = serializer->topic;
	serializer->topic_ownership = 0;
	message->data = serializer->data;
	serializer->data_ownership = 0;
	packet->streamed = serializer->stream;
	serializer->stream_ownership = 0;
	message->duplicate = serializer->duplicate;
	message->retain = serializer->retain;
	message->quality_of_service = serializer->quality_of_service;
	message->message_id = serializer->message_id;

	if(
		(packet->type == CONNECT) ||
		(packet->type == CONNACK) ||
		((packet->type == PUBLISH) && (message->quality_of_service == 0)) ||
		(packet->type == PINGREQ) ||
		(packet->type == PINGRESP) ||
		(packet->type == DISCONNECT)
	)
		packet->packet_id = -1;

	pico_mqtt_serializer_clear(serializer);

	return packet;
}

/**
* Private functions implementation
**/

static int create_stream( struct pico_mqtt_serializer* serializer, const uint32_t length )
{
	CHECK_NOT_NULL(serializer);
	CHECK_NULL(serializer->stream.data);
	CHECK(length != 0, "Attempting to create an zero length stream.\n");
	CHECK((serializer->stream.length == 0),
		"The stream is not freed correct yet, by calling create stream a memory leak can be created.\n");

	if(serializer->stream_ownership)
	{
		PWARNING("The output stream is set but not yet used.\n");
		FREE(serializer->stream.data);
	}

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

	length = serializer->stream.length -
		((uint8_t*)(serializer->stream_index) - (uint8_t*)(serializer->stream.data));
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
	serializer->stream_index = (uint8_t*)(serializer->stream_index) + 1;
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
	serializer->stream_index = (uint8_t*)(serializer->stream_index) + 1;
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

static struct pico_mqtt_data* get_data_stream( struct pico_mqtt_serializer* serializer )
{
	uint32_t index = 0;
	uint32_t bytes_left = 0;
	struct pico_mqtt_data* result = NULL;

	CHECK_NOT_NULL( serializer );
	bytes_left = get_bytes_left_stream(serializer);
	if(bytes_left == 0)
	{
		PINFO("No bytes left in the stream, please check before requesting data.\n");
		return NULL;
	}

	result = (struct pico_mqtt_data*)MALLOC(sizeof(struct pico_mqtt_data));
	if(result == NULL)
	{
		PERROR("Failed to allocate the memory needed.\n");
		return NULL;
	}

	result->data = MALLOC(bytes_left);
	if(result->data == NULL)
	{
		PERROR("Failed to allocate the memory needed.\n");
		FREE(result);
		return NULL;
	}

	result->length = bytes_left;
	for(index = 0; index < bytes_left; ++index)
	{
		*(((uint8_t*)(result->data)) + index) = get_byte_stream( serializer );
	}

	return result;
}

static void add_data_and_length_stream( struct pico_mqtt_serializer* serializer, const struct pico_mqtt_data* data)
{
	if(data == NULL)
		return;

	CHECK((data->length) <= 0xFFFF,
		"Can not add data longer than 65535.\n");

	add_2_byte_stream(serializer, (uint16_t) data->length);

	if(data->length != 0)
		add_data_stream(serializer, *data);

	return;
}

static struct pico_mqtt_data* get_data_and_length_stream( struct pico_mqtt_serializer* serializer )
{
	uint32_t index = 0;
	int32_t bytes_left = 0;
	uint16_t length = 0;
	struct pico_mqtt_data* result = NULL;
	CHECK_NOT_NULL(serializer);

	bytes_left = (int32_t) get_bytes_left_stream(serializer);
	if(bytes_left < 2)
	{
		PTODO("Set appropriate error.\n");
		PERROR("Message is not long enough to read a 2 byte length field.\n");
		return NULL;
	}

	length = get_2_byte_stream(serializer);
	if(bytes_left < length + 2)
	{
		PTODO("Set appropriate error.\n");
		PERROR("Invallid length, the stream is not long enough to contain the indicated data.\n");
		return NULL;
	}

	result = (struct pico_mqtt_data*) MALLOC(sizeof(struct pico_mqtt_data));
	if(result == NULL)
	{
		PERROR("Failed to allocate the memory needed.\n");
		return NULL;
	}

	if(bytes_left == 2)
	{
		result->data = NULL;
		result->length = 0;
		return result;
	}

	result->data = MALLOC(length);
	if(result->data == NULL)
	{
		FREE(result);
		PERROR("Failed to allocate the memory needed.\n");
		return NULL;
	}

	result->length = length;
	for(index = 0; index < length; ++index)
	{
		*(((uint8_t*)(result->data)) + index) = get_byte_stream( serializer );
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

	CHECK(!((serializer->topic == NULL) && (serializer->data != NULL)),
		"If the will topic is not set the will message must not be set.\n");

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

	if(create_stream( serializer, length + 1 + serializer->serialized_length.length) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	add_byte_stream( serializer, fixed_header );
	add_data_stream( serializer, serializer->serialized_length);
	add_array_stream( serializer, variable_header, 10);

	add_data_and_length_stream( serializer, serializer->client_id);
	add_data_and_length_stream( serializer, serializer->topic);
	if((serializer->data == NULL) && (serializer->topic != NULL))
	{
		add_2_byte_stream(serializer, 0);
	} else
	{
		add_data_and_length_stream( serializer, serializer->data);
	}
	add_data_and_length_stream( serializer, serializer->username);
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

	PTODO("Check for wildcards or system topics, check if topic has length 0\n");

	if(serializer->topic == NULL)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("A topic is required when publishing a message.\n");
		return ERROR;
	}

	if(serializer->data == NULL)
	{
		length = serializer->topic->length + 2;
	} else
	{
		length = serializer->topic->length + 2 + serializer->data->length;
	}

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

	if(create_stream( serializer, length + 1 + serializer->serialized_length.length) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	add_byte_stream( serializer, fixed_header );
	add_data_stream( serializer, serializer->serialized_length);
	add_data_and_length_stream( serializer, serializer->topic);

	if(serializer->quality_of_service != 0)
	{
		add_2_byte_stream( serializer, serializer->message_id );
	}

	if(serializer->data != NULL)
	{
		if((serializer->data->data != NULL) && (serializer->data->length != 0))
			add_data_stream( serializer, *(serializer->data));
	}

	return SUCCES;
}

static int serialize_puback( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_acknowledge( serializer, PUBACK_FLAG );
}

static int serialize_pubrec( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_acknowledge( serializer, PUBREC_FLAG );
}

static int serialize_pubrel( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_acknowledge( serializer, PUBREL_FLAG );
}

static int serialize_pubcomp( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);

	return serialize_acknowledge( serializer, PUBCOMP_FLAG );
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
		(packet_type == PUBACK_FLAG) ||
		(packet_type == PUBREC_FLAG) ||
		(packet_type == PUBREL_FLAG) ||
		(packet_type == PUBCOMP_FLAG)||
		(packet_type == UNSUBACK_FLAG)
		,"The requested packet type cannot be serialized as an acknowledge message.\n");

	if(packet_type != PUBREL_FLAG) /* table 2.2 - Flag Bits */
	{
		flags = 0x00;
	} else
	{
		flags = 0x02;
	}

	if(create_stream( serializer, 4 ) == ERROR)
	{
		PTRACE();
		return ERROR;
	}

	packet[0] |= packet_type;
	packet[0] |= flags;
	PTODO("Check little and big endian.\n");
	packet[2] |= (uint8_t)((serializer->message_id >> 8) & 0x00FF); /* MSB */
	packet[3] |= (uint8_t)(serializer->message_id & 0x00FF); /* LSB */
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
	if(serializer->client_id == NULL)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The client_id is not set but is obliged.\n");
		return ERROR;
	}
#endif /* ALLOW_EMPTY_CLIENT_ID == 0 */

	if((serializer->username == NULL) && (serializer->password != NULL))
	{
			PTODO("set appropriate error.\n");
			PERROR("Password can not be set without username.\n");
			return ERROR;
	}

	if((serializer->topic == NULL) && (serializer->data != NULL))
	{
			PTODO("set appropriate error.\n");
			PERROR("Payload cannot be set without a topic.\n");
			return ERROR;
	}

	return SUCCES;
}

static uint32_t serialize_connect_get_length( struct pico_mqtt_serializer* serializer)
{
	uint32_t length = 12; /* add 10 length for standard variable header +2 cliend id length*/

	CHECK_NOT_NULL(serializer);

	if(serializer->client_id != NULL)
		length += serializer->client_id->length;

	if(serializer->username != NULL){
		length += serializer->username->length + 2;

		if(serializer->password != NULL)
			length += serializer->password->length + 2;
	}

	if(serializer->topic != NULL)
		length += serializer->topic->length + 2 + 2;

	if(serializer->data != NULL)
		length += serializer->data->length;

	return length;
}

static uint8_t serialize_connect_get_flags( struct pico_mqtt_serializer* serializer)
{
	uint8_t flags = 0;

	CHECK_NOT_NULL(serializer);

	if(serializer->username != NULL){
		flags |= 0x80; /* User Name Flag */

		if(serializer->password != NULL)
			flags |= 0x40; /* password Flag */
	}

	if((serializer->clean_session != 0) ||
		(serializer->client_id == NULL))
		flags |= 0x02; /* Clean Session */

	if(	(serializer->topic != NULL) &&
		(serializer->data != NULL))
	{
		flags |= 0x04; /* Will Flag */
		flags |= (uint8_t)((serializer->quality_of_service & 0x03) << 3); /* Will QoS */

		if(serializer->retain != 0)
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

	CHECK( byte >> 4 == 0x02, "The message is not a CONNACK_FLAG message as expected.\n");
	CHECK( byte == 0x20, "The CONNACK_FLAG message doesn't have the correct fixed header flags set.\n");

	if(serializer->stream.length != 4)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the CONNACK_FLAG message is not correct.\n");
		return ERROR;
	}

	byte = get_byte_stream( serializer );

	if( byte != 2)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the CONNACK_FLAG message is not correct.\n");
		return ERROR;
	}

	byte = get_byte_stream( serializer );

	CHECK((byte & 0xFE) == 0x00, "The CONNACK_FLAG message doesn't have the correct acknowledge flags set.\n");

	serializer->session_present = (byte & 0x01) == 0x01;

	serializer->return_code = get_byte_stream( serializer );

	if(serializer->return_code > 5)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The return code of the CONNACK_FLAG message is unknown.\n");
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
		PERROR("The length of the PUBLISH_FLAG message is to short to be valid.\n");
		return ERROR;
	}

	serializer->message_type = 0x03;
	byte = get_byte_stream(serializer);

	CHECK((byte >> 4 == 0x03), "The message is not a PUBLISH_FLAG message as expected.\n");

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
	if(serializer->topic == NULL)
	{
		PTODO("set appropriate error\n");
		PERROR("unable to allocate memory.\n");
		return ERROR;
	}
	serializer->topic_ownership = 1;


	if(serializer->topic->length == 0)
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The topic cannot be 0-length.\n");
		return ERROR;
	}

	if(serializer->quality_of_service != 0)
		serializer->message_id = get_2_byte_stream(serializer);

	serializer->data = get_data_stream( serializer );
	if(serializer->data == NULL)
	{
		PTODO("set appropriate error\n");
		PERROR("unable to allocate memory.\n");
		return ERROR;
	}
	serializer->data_ownership = 1;

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

	serializer->message_id = get_2_byte_stream( serializer );

	return SUCCES;
}

static int deserialize_suback(struct pico_mqtt_serializer* serializer )
{
	uint8_t byte = 0;
	CHECK_NOT_NULL(serializer);

	serializer->message_type = 0x09;
	byte = get_byte_stream( serializer );

	CHECK( byte >> 4 == 0x09, "The message is not a SUBACK_FLAG message as expected.\n");
	CHECK( byte == 0x90, "The SUBACK_FLAG message doesn't have the correct fixed header flags set.\n");

	if(serializer->stream.length != 5)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the SUBACK_FLAG message is not correct.\n");
		return ERROR;
	}

	byte = get_byte_stream( serializer );

	if( byte != 3)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The length of the SUBACK_FLAG message is not correct.\n");
		return ERROR;
	}

	serializer->message_id = get_2_byte_stream( serializer );

	byte = get_byte_stream( serializer );

	if( byte == 0x80 )
	{
		PTODO("Set appropriate error.\n");
		PERROR("The SUBACK_FLAG indicated a failure.\n");
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

	if(serializer->topic == NULL)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The topic should be set.\n");
		return ERROR;
	}

	pico_mqtt_serialize_length( serializer, serializer->topic->length + 5 );

	if(create_stream( serializer, serializer->topic->length + 6 + serializer->serialized_length.length) == ERROR)
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
	add_2_byte_stream( serializer, serializer->message_id);
	add_data_and_length_stream( serializer, serializer->topic );
	add_byte_stream( serializer, serializer->quality_of_service );

	return SUCCES;
}

static int serialize_pingreq( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);
	if(create_stream(serializer, 2) == ERROR)
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
	if(create_stream(serializer, 2) == ERROR)
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
	if(create_stream(serializer, 2) == ERROR)
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
		PERROR("The PINGREQ_FLAG packet should be 2 byte long, not %d.\n", serializer->stream.length);
		return ERROR;
	}

	if(get_2_byte_stream(serializer) != 0xC000)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The PINGREQ_FLAG packet is not formed correctly.\n");
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
		PERROR("The PINGRESP_FLAG packet should be 2 byte long, not %d.\n", serializer->stream.length);
		return ERROR;
	}

	if(get_2_byte_stream(serializer) != 0xD000)
	{
		PTODO("Set appropriate error.\n");
		PERROR("The PINGRESP_FLAG packet is not formed correctly.\n");
		return ERROR;
	}

	return SUCCES;
}
