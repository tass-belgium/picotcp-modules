#include "pico_mqtt_serializer.h"
#include "pico_mqtt_error.h"

/**
* Lengths
**/

#define FIXED_HEADER_SIZE 1
#define LENGTH_SIZE 2
#define PACKET_ID_SIZE 2
#define FLAG_SIZE 1
#define RETURN_CODE_SIZE 1
#define QUALITY_OF_SERVICE_SIZE 1
#define MESSAGE_ID_SIZE 2

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

/**
* Local function prototypes
**/
static int create_header(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint8_t fixed_header
	);

static int create_acknowledge(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint8_t fixed_header,
	uint16_t packet_id
	);

static int create_message(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	size_t length,
	void** stream,
	uint8_t fixed_header,
	uint16_t message_id);

static inline void set_publish_flags(
	uint8_t* fixed_header,
	uint8_t retain,
	uint8_t quality_of_service);

static inline void set_connect_flags(
	uint8_t* header,
	uint8_t username,
	uint8_t password,
	uint8_t retain,
	uint8_t quality_of_service,
	uint8_t will,
	uint8_t clean_session);

static inline uint16_t get_next_message_id(struct pico_mqtt* mqtt);
static uint16_t get_data_length(struct pico_mqtt_data* data);
static uint32_t get_buffer_length(struct pico_mqtt_data* data);
static int convert_length(struct pico_mqtt_data** converted, uint32_t length);
static inline void destroy_length(struct pico_mqtt_data* converted_length);
static inline void add_header_stream(void** stream, uint8_t fixed_header, struct pico_mqtt_data* length);
static inline void add_message_id_stream(void** stream, uint16_t message_id);
static inline void add_quality_of_service_stream(void** stream, uint8_t quality_of_service);
static inline void add_payload_stream(void** stream, struct pico_mqtt_data* data);
static inline void add_data_and_length_stream(void** stream, struct pico_mqtt_data* data);
static void add_data_stream(void** stream, void* data, uint32_t length);

/**
* Deserialization Funcitons
**/

static int deserialize_connect( struct pico_mqtt* mqtt, struct pico_mqtt_message* message);

/**
* Message types
**/

__attribute__((packet)) struct connect_header{
	uint8_t length_MSB;
	uint8_t length_LSB;
	char protocol[4];
	uint8_t version;
	uint8_t flags;
	uint16_t keep_alive;
};

__attribute__((packed)) struct acknowledge{
	uint8_t fixed_header;
	uint8_t remaining_length;
	uint16_t packet_id;
};

#ifdef DEBUG
__attribute__((packed)) struct connack{
	uint8_t fixed_header;
	uint8_t length;
	uint8_t flag;
	uint8_t return_code;
};
#endif

#ifdef DEBUG
__attribute__((packed)) struct suback{
	uint8_t fixed_header;
	uint8_t length;
	uint16_t packet_id;
	uint8_t return_code;
};
#endif

__attribute__((packed)) struct header{
	uint8_t fixed_header;
	uint8_t length;
};

/**
* Public Function Implemantation
**/

int pico_mqtt_serializer_create_connect( 
	struct pico_mqtt* mqtt, 
	struct pico_mqtt_message** result, 
	uint16_t keep_alive_time,
	uint8_t retain,
	uint8_t quality_of_service,
	uint8_t clean_session,
	struct pico_mqtt_data* client_id,
	struct pico_mqtt_data* topic,
	struct pico_mqtt_data* message,
	struct pico_mqtt_data* username,
	struct pico_mqtt_data* password
	)
{
	uint32_t length = 0;
	uint32_t total_length = 0;
	void* stream = NULL;
	uint8_t fixed_header = CONNECT;
	struct pico_mqtt_data* converted_length = NULL;
	struct connect_header header = {
		.length_MSB = 0,
		.length_LSB = 4,
		.protocol = {'M','Q','T','T'},
		.version = 4,
		.flags = 0,
		.keep_alive = keep_alive_time
	};

#ifdef DEBUG /* this check should only be performed during debug */
	if(
		(mqtt == NULL) || 
		(result == NULL) || 
#if ALLOW_EMPTY_CLIENT_ID == 0
		(client_id == NULL) || 
#endif /* ALLOW_EMPTY_CLIENT_ID == 0 */
		(retain > 1) || 
		(quality_of_service > 2) || 
		(clean_session > 1) ||
		((topic == NULL) && (message != NULL)) ||
		((topic != NULL) && (message == NULL)))
	{
		PERROR("Invallid input parameters.\n");
		return ERROR;
	}
#endif

	if((mqtt->password != NULL) && (mqtt->username == NULL))
	{
		PERROR("Not allowed to set a password without setting the username.\n");
		return ERROR;
	}

	set_connect_flags(
		&(header.flags), 
		(username != NULL), 
		(password != NULL), 
		(retain != 0), 
		quality_of_service, 
		(topic != NULL),
		(clean_session != 0));

	length += sizeof(struct connect_header);
	length += get_buffer_length(client_id);
	length += get_buffer_length(topic);
	length += get_buffer_length(message);
	length += get_buffer_length(username);
	length += get_buffer_length(password);

	if(convert_length(&converted_length, length))
	{
		PTRACE;
		return ERROR;
	}

	total_length = FIXED_HEADER_SIZE + converted_length->length + length;
	if(create_message(mqtt, result, total_length, &stream, fixed_header, 0) != SUCCES)
	{
		destroy_length(converted_length);
		PTRACE;
		return ERROR;
	}

	PINFO("Succesfully created the connect package.\n");

	add_header_stream(&stream, fixed_header, converted_length);
	add_data_stream(&stream, (void*) &header, sizeof(struct connect_header));
	add_data_and_length_stream(&stream, client_id);
	add_data_and_length_stream(&stream, topic);
	add_data_and_length_stream(&stream, message);
	add_data_and_length_stream(&stream, username);
	add_data_and_length_stream(&stream, password);
	
	destroy_length(converted_length);

	return SUCCES;
}


#ifdef DEBUG
int deserialize_connect( 
	struct pico_mqtt* mqtt, 
	struct pico_mqtt_data raw, 
	uint16_t* keep_alive_time,
	uint8_t* retain,
	uint8_t* quality_of_service,
	uint8_t* clean_session,
	struct pico_mqtt_data* client_id,
	struct pico_mqtt_data* topic,
	struct pico_mqtt_data* message,
	struct pico_mqtt_data* username,
	struct pico_mqtt_data* password)
{
	struct pico_mqtt_data stream = raw;

#ifdef DEBUG
	if( (mqtt == NULL) || 
		(raw.data == NULL) || 
		(keep_alive_time == NULL) ||
		(retain == NULL) ||
		(quality_of_service == NULL) ||
		(clean_session == NULL) ||
		(client_id == NULL) ||
		(topic == NULL) ||
		(message == NULL) ||
		(username == NULL) ||
		(password == NULL))
	{
		PERROR("Invallid arguments (%p, %p, %p, %p,\n %p, %p, %p, %p,\n %p, %p, %p).\n", 
			mqtt,
			raw.data,
			keep_alive_time,
			retain,

			quality_of_service,
			clean_session,
			client_id,
			topic,

			message,
			username,
			password);
		return ERROR;
	}
#endif


	get_header_stream()
	

	return SUCCES;
}
#endif /* define DEBUG */

#ifdef DEBUG /* only needed for debug purposes*/
int pico_mqtt_create_connack(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint8_t session_present,
	uint8_t return_code
	)
{
	void* stream = NULL;
	struct connack message = 
	{
		.fixed_header = CONNACK,
		.length = FLAG_SIZE + RETURN_CODE_SIZE,
		.flag = session_present,
		.return_code = return_code
	};

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL) || (session_present > 1) || (return_code > 5))
	{
		PTRACE;
		return ERROR;
	}
#endif

	if(create_message(mqtt, result, sizeof(struct connack), &stream, message.fixed_header, 0) != SUCCES)
	{
		PTRACE;
		return ERROR;
	}

	add_data_stream(&stream, (void*) &message, sizeof(struct connack));
	return SUCCES;	
}
#endif

int pico_mqtt_create_publish(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result, 
	uint8_t quality_of_service,
	uint8_t retain,
	struct pico_mqtt_data* topic,
	struct pico_mqtt_data* data
	)
{
	uint8_t fixed_header = PUBLISH;
	uint32_t length = 0;
	uint32_t total_length = 0;
	uint16_t message_id = 0;
	struct pico_mqtt_data* converted_length = NULL;
	void* stream = NULL;

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL) || (topic == NULL) || (quality_of_service > 2) || (retain > 1))
	{
		PERROR("Invallid input.\n");
		return ERROR;
	}
#endif
	set_publish_flags(&fixed_header, retain, quality_of_service);

	length += get_buffer_length(topic);
	if(quality_of_service > 0)
	{
		length += MESSAGE_ID_SIZE;
		message_id = get_next_message_id(mqtt);
	}
	length += get_data_length(data);

	if(convert_length(&converted_length, length))
	{
		PTRACE;
		return ERROR;
	}

	total_length = FIXED_HEADER_SIZE + converted_length->length + length;
	if(create_message(mqtt, result, total_length, &stream, fixed_header, message_id) != SUCCES)
	{
		destroy_length(converted_length);
		return ERROR;
	}

	/* add data to stream */
	add_header_stream(&stream, fixed_header, converted_length);
	add_data_and_length_stream(&stream, topic);
	if(quality_of_service > 0)
	{
		add_message_id_stream(&stream, (*result)->message_id);
	}
	add_payload_stream(&stream, data);

	/* destroy the length object */
	destroy_length(converted_length);

	return SUCCES;	
}

int pico_mqtt_create_puback(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	)
{
	return create_acknowledge(mqtt, result, PUBACK, packet_id);
}

int pico_mqtt_create_pubrec(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	)
{
	return create_acknowledge(mqtt, result, PUBREC, packet_id);
}

int pico_mqtt_create_pubrel(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	)
{
	return create_acknowledge(mqtt, result, PUBREL | QUALITY_OF_SERVICE_1, packet_id);
}

int pico_mqtt_create_pubcomp(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	)
{
	return create_acknowledge(mqtt, result, PUBCOMP, packet_id);
}

int pico_mqtt_create_subscribe(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	struct pico_mqtt_data* topic,
	uint8_t quality_of_service
	)
{
	uint8_t fixed_header = SUBSCRIBE | QUALITY_OF_SERVICE_1;
	void* stream = NULL;
	struct pico_mqtt_data* converted_length = NULL;
	uint32_t length = 0;
	uint32_t total_length = 0;
	uint16_t message_id = 0;

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL) || (topic == NULL) || (quality_of_service > 2))
	{
		PTRACE;
		return ERROR;
	}
#endif
	message_id = get_next_message_id(mqtt);

	length += MESSAGE_ID_SIZE;
	length += get_buffer_length(topic);
	length += QUALITY_OF_SERVICE_SIZE;


	if(convert_length(&converted_length, length))
	{
		PTRACE;
		return ERROR;
	}

	total_length = FIXED_HEADER_SIZE + converted_length->length + length;
	if(create_message(mqtt, result, total_length, &stream, fixed_header, message_id) != SUCCES)
	{
		destroy_length(converted_length);
		return ERROR;
	}

	add_header_stream(&stream, fixed_header, converted_length);
	add_message_id_stream(&stream, message_id);
	add_data_and_length_stream(&stream, topic);
	add_quality_of_service_stream(&stream, quality_of_service);

	/* destroy the length object */
	destroy_length(converted_length);

	return SUCCES;	
}

#ifdef DEBUG /* only needed for debug purposes*/
int pico_mqtt_create_suback(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id,
	uint8_t return_code
	)
{
	void* stream = NULL;
	struct suback message = 
	{
		.fixed_header = SUBACK,
		.length = PACKET_ID_SIZE + RETURN_CODE_SIZE,
		.packet_id = packet_id,
		.return_code = return_code
	};

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL) || ((return_code > 2) && (return_code != 0x80)))
	{
		PTRACE;
		return ERROR;
	}
#endif

	if(create_message(mqtt, result, sizeof(struct suback), &stream, message.fixed_header, packet_id) != SUCCES)
	{
		PTRACE;
		return ERROR;
	}

	add_data_stream(&stream, (void*) &message, sizeof(struct suback));
	
	return SUCCES;	
}
#endif

int pico_mqtt_create_unsubscribe(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	struct pico_mqtt_data* topic
	)
{
	void* stream = NULL;
	uint8_t fixed_header = UNSUBSCRIBE | QUALITY_OF_SERVICE_1;
	struct pico_mqtt_data* converted_length = NULL;
	uint32_t length = PACKET_ID_SIZE;
	uint32_t total_length = 0;
	uint16_t message_id = 0;

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL) || (topic == NULL))
	{
		PTRACE;
		return ERROR;
	}
#endif
	length += get_buffer_length(topic);
	message_id = get_next_message_id(mqtt);

	if(convert_length(&converted_length, length))
	{
		PTRACE;
		return ERROR;
	}

	total_length = FIXED_HEADER_SIZE + converted_length->length + length;
	if(create_message(mqtt, result, total_length, &stream, fixed_header, message_id) != SUCCES)
	{
		destroy_length(converted_length);
		return ERROR;
	}

	add_header_stream(&stream, fixed_header, converted_length);
	add_message_id_stream(&stream, message_id);
	add_data_and_length_stream(&stream, topic);
	
	/* destroy the length object */
	destroy_length(converted_length);

	return SUCCES;	
}

#ifdef DEBUG  /* only needed for debug purposes*/
int pico_mqtt_create_unsuback(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	)
{
	return create_acknowledge(mqtt, result, UNSUBACK, packet_id);
}
#endif

int pico_mqtt_create_pingreq(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result
	)
{
	return create_header(mqtt, result, PINGREQ);
}

#ifdef DEBUG  /* only needed for debug purposes*/
int pico_mqtt_create_pingresp(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result
	)
{
	return create_header(mqtt, result, PINGRESP);
}
#endif

int pico_mqtt_create_disconnect(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result
	)
{
	return create_header(mqtt, result, DISCONNECT);
}

/**
* Deserializer Implementation
**/

int pico_mqtt_deserialize(
        struct pico_mqtt* mqtt,
        struct pico_mqtt_message* message
        )
{
	int result = 0;
	uint8_t message_type;
	int (*deserializer_function)(struct pico_mqtt* mqtt, struct pico_mqtt_message* message);
	
	/**
	* Lookup table for custom deserialization funcitons
	*
	* NULL means not yet defined
	**/

#ifdef DEBUG
	static int (* const deserializer_functions[16])(struct pico_mqtt* mqtt, struct pico_mqtt_message* message) =
	{
		NULL, /* invallid packed ID */
		NULL, /* CONNECT - not supported */
		deserialize_connect, /* CONNACK */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};
#else
	static int (* const deserializer_functions[16])(struct pico_mqtt* mqtt, struct pico_mqtt_message* message) =
	{
		NULL, /* invallid packet ID */
		NULL, /* CONNECT - not supported */
		deserialize_connect, /* CONNACK */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	};
#endif /* defined DEBUG */

#ifdef DEBUG
	if((mqtt == NULL) || (message == NULL))
	{
		PERROR("Invallid input.\n");
		return ERROR;
	}
#endif
	
	message_type = (message->header) >> 4;
	deserializer_function = deserializer_functions[message_type];

	if(deserializer_function == NULL)
	{
		PERROR("Deserializing a message of type %d is not supported.\n", message_type);
		return ERROR;
	}

	result = deserializer_function(mqtt, message);
	if(result == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	return SUCCES;
}

/**
* Private Function Implementation
**/

static int create_header(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint8_t fixed_header
	)
{
	void* stream = NULL;
	struct header message = 
	{
		.fixed_header = fixed_header,
		.length = 0
	};

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL))
	{
		PTRACE;
		return ERROR;
	}
#endif

	if(create_message(mqtt, result, sizeof(struct header), &stream, fixed_header, 0) != SUCCES)
	{
		PTRACE;
		return ERROR;
	}

	add_data_stream(&stream, (void*) &message, sizeof(struct header));
	return SUCCES;	
}

static int create_acknowledge(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint8_t fixed_header,
	uint16_t packet_id
	)
{
	void* stream = NULL;
	struct acknowledge message = 
	{
		.fixed_header = fixed_header,
		.remaining_length = PACKET_ID_SIZE,
		.packet_id = packet_id
	};

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL))
	{
		PTRACE;
		return ERROR;
	}
#endif

	if(create_message(mqtt, result, sizeof(struct acknowledge), &stream, fixed_header, 0) != SUCCES)
	{
		PTRACE;
		return ERROR;
	}

	add_data_stream(&stream, (void*) &message, sizeof(struct acknowledge));
	return SUCCES;	
}

static int create_message(struct pico_mqtt* mqtt, struct pico_mqtt_message** result, size_t length, void** stream, uint8_t fixed_header, uint16_t message_id)
{
#ifdef DEBUG
	if( (mqtt == NULL) || (result == NULL) || (stream == NULL))
	{
		PERROR("invallid input parameters.\n");
		return ERROR;
	}
#endif

	PTODO("Use a global method to create the message.\n");
	*result = (struct pico_mqtt_message*) malloc(sizeof(struct pico_mqtt_message));
	**result = (struct pico_mqtt_message) {
		.header = fixed_header,
		.status = 0,
		.message_id = message_id,
		.topic = {.length = 0, .data = NULL},
		.data = {.length = 0, .data = NULL}};

	/*
	if(mqtt->malloc(mqtt, &((*result)->data.data), length) != SUCCES)
	{
		PTRACE;
		return ERROR;
	}
	*/
	(*result)->data.data = malloc(length);

	(*result)->data.length = length;
	*stream = (*result)->data.data;
	return SUCCES;
}

static inline void set_publish_flags(uint8_t* fixed_header, uint8_t retain, uint8_t quality_of_service)
{
	if(retain != 0)
	{
		*fixed_header |= RETAIN;
	}

	if(quality_of_service == 1)
	{
		*fixed_header |= QUALITY_OF_SERVICE_1;
	}

	if(quality_of_service == 2)
	{
		*fixed_header |= QUALITY_OF_SERVICE_2;
	}
}

static inline void set_connect_flags(
	uint8_t* header,
	uint8_t username,
	uint8_t password,
	uint8_t retain,
	uint8_t quality_of_service,
	uint8_t will,
	uint8_t clean_session)
{
	*header = 0;
	*header |= username << 7;
	*header |= password << 6; 
	*header |= retain << 5;
	*header |= quality_of_service << 3;
	*header |= will << 2;
	*header |= clean_session << 1;
}

static inline uint16_t get_next_message_id(struct pico_mqtt* mqtt)
{
	return (mqtt->next_message_id)++;
}

/* only get the length of the data itself */
static uint16_t get_data_length(struct pico_mqtt_data* data)
{
	if(data == NULL)
		return 0;
	
	return data->length;
}

/* get the data length with the sizeof the length */
static uint32_t get_buffer_length(struct pico_mqtt_data* data)
{
	if(data == NULL)
		return 0;

	return data->length + LENGTH_SIZE;
}

static int convert_length(struct pico_mqtt_data** converted, uint32_t length)
{
	PTODO("cleanup this function.\n");
	PTODO("check where this memory is reclaimed");

	struct pico_mqtt_data* data = NULL;
	data = (struct pico_mqtt_data*) malloc(sizeof(struct pico_mqtt_data));
	if(data == NULL)
	{
		PERROR("Unable to allocate memory.\n");
		return ERROR;
	}
	int8_t i;
	uint32_t mask = 0x00200000;
	uint8_t continue_flag = 0;
	data->length = 0;

	if(converted == NULL)
	{
		PERROR("Invallid input, receive NULL.\n");
		return ERROR;
	}

	if(length >= 0x10000000){
		PERROR("Requested size to long.\n");
		return ERROR;
	}

	data->length = 0;
	for(i = 3; i >=0; --i){
		if(length >= mask){
			if(data->length == 0){
				data->length = i+1;
				data->data = malloc(data->length);
				if(data->data == NULL)
				{
					PERROR("Unable to allocate memory.\n");
					return ERROR;
				}
			}
			
			*(((uint8_t*) data->data) + i) = length / mask;
			length = length % mask;
			if(continue_flag == 1)
				*(((uint8_t*) data->data) + i) |= 0x80;
			continue_flag = 1;
		}
		mask = mask >> 7;
	}

	*converted = data;
	return SUCCES;	
}

static inline void destroy_length(struct pico_mqtt_data* converted_length)
{
	free(converted_length->data);
	free(converted_length);
}

static inline void add_header_stream(void** stream, uint8_t fixed_header, struct pico_mqtt_data* length)
{
	if(length != NULL)
	{
		add_data_stream(stream, (void*) &fixed_header, FIXED_HEADER_SIZE);
		add_data_stream(stream, length->data, length->length);
	}
}

static inline void add_message_id_stream(void** stream, uint16_t message_id)
{
	add_data_stream(stream, (void*) &message_id, MESSAGE_ID_SIZE);
}

static inline void add_quality_of_service_stream(void** stream, uint8_t quality_of_service)
{
	add_data_stream(stream, (void*) &quality_of_service, QUALITY_OF_SERVICE_SIZE);
}

static inline void add_payload_stream(void** stream, struct pico_mqtt_data* data)
{
	if(data != NULL)
		add_data_stream(stream, data->data, data->length);
}

static inline void add_data_and_length_stream(void** stream, struct pico_mqtt_data* data)
{
	PTODO("Encode litle and big endian.\n");
	if(data != NULL)
	{
		add_data_stream(stream, (uint8_t*) &(data->length) + 1, 1);
		add_data_stream(stream, (uint8_t*) &(data->length), 1);
		add_data_stream(stream, data->data, data->length);
	}
}

static void add_data_stream(void** stream, void* data, uint32_t length)
{
	uint32_t i = 0;
	for(i=0; i<length; ++i){
		**((uint8_t**)stream) = *((uint8_t*)data);
		(*((uint8_t**)stream))++;
		++data;
	}
}
