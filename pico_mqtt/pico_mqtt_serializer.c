#include "pico_mqtt_serializer.h"

/**
* Error related
**/

#define SUCCES 0
#define FAIL -1

/**
* Lengths
**/

#define FIXED_HEADER_SIZE 1
#define LENGTH_SIZE 2
#define PACKET_ID_SIZE 2
#define FLAG_SIZE 1
#define RETURN_CODE_SIZE 1
#define QUALITY_OF_SERVICE_SIZE 1

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
* Connack flags - debug only
**/

#ifdef DEBUG

	#define SESSION_PRESENT 1
	#define NO_SESSION_PRESENT 0
	#define CONNECTION_ACCEPTED 0
	#define UNACCEPTABLE_PROTOCOL_VERSION 1
	#define IDENTIFIER_REJECTED 2
	#define SERVER_UNAVAILABLE 3
	#define BAD_CREDENTIALS 4
	#define NOT_AUTHORIZED 5

#endif

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
* Message types
**/

__atribute__((packet)) struct connect{
	uint16_t length;
	char protocol[4];
	uint8_t version;
	uint8_t flags;
	uint16_t keep_alive;
};

__atribute__((packed)) struct acknowledge{
	uint8_t fixed_header;
	uint8_t length;
	uint16_t packet_id;
};

#ifdef DEBUG
__atribute__((packed)) struct connack{
	uint8_t fixed_header;
	uint8_t length;
	uint8_t flag;
	uint8_t return_code;
};
#endif

#ifdef DEBUG
__atribute__((packed)) struct suback{
	uint8_t fixed_header;
	uint8_t length;
	uint16_t packet_id;
	uint8_t return_code;
};
#endif

__atribute__((packed)) struct header{
	uint8_t fixed_header;
	uint8_t length;
};

int pico_mqtt_serializer_create_connect( 
	struct pico_mqtt* mqtt, 
	struct pico_mqtt_message** return_message, 
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
	struct connect header = {
		.length = 4,
		.protocol = {'M','Q','T','T'},
		.version = 4,
		.flags = 0,
		.keep_alive = keep_alive_time
	};

#ifdef DEBUG /* this check should only be performed during debug */
	if(
		(mqtt == NULL) || 
		(return_message == NULL) || 
		(client_id == NULL) || 
		(retain > 1) || 
		(quality_of_service > 2) || 
		(clean_session > 1) ||
		((topic == NULL) && (message != NULL)) ||
		((topic != NULL) && (message == NULL)))
	{
		return FAIL;
	}
#endif

	set_connect_flags(
		&(header.flags), 
		(username != NULL), 
		(password != NULL), 
		(retain != NULL), 
		quality_of_service, 
		(topic != NULL),
		(clean_session != NULL));

	length += sizeof(struct connect);
	length += get_buffer_length(client_id);
	length += get_buffer_length(topic);
	length += get_buffer_length(message);
	length += get_buffer_length(username);
	length += get_buffer_length(password);

	if(convert_length(&converted_length, length))
	{
		return FAIL;
	}

	total_length = FIXED_HEADER_SIZE + converted_length->length + length;
	if(create_message(mqtt, result, total_length, &stream, fixed_header, 0) != SUCCES)
	{
		destroy_length(converted_length);
		return FAIL;
	}

	add_header_stream(&stream, fixed_header, converted_length);
	add_data_stream(&stream, (void*) &header, sizeof(struct connect));
	add_data_and_length_stream(&stream, client_id);
	add_data_and_length_stream(&stream, topic);
	add_data_and_length_stream(&stream, message);
	add_data_and_length_stream(&stream, username);
	add_data_and_length_stream(&stream, password);
	
	destroy_length(converted_length);

	return SUCCES;
}

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
		.remaining_length = FLAG_SIZE + RETURN_CODE_SIZE,
		.flag = sessiont_present,
		.return_code = return_code
	};

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL) || (session_present > 1) || (return_code > 5))
	{
		return FAIL;
	}
#endif

	if(create_message(mqtt, result, sizeof(struct connack), &stream, message.fixed_header, 0) != SUCCES)
	{
		return FAIL;
	}

	add_data_stream(&stream, (void*) &message, sizeof(struct connack));
	return SUCCES;	
}
#endif

int pico_mqtt_create_publish(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** return_message, 
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
	if((mqtt == NULL) || (return_message == NULL) || (topic == NULL) || (quality_of_service > 2) || (retain > 1))
	{
		return FAIL;
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
		return FAIL;
	}

	total_length = FIXED_HEADER_SIZE + converted_length->length + length;
	if(create_message(mqtt, result, total_length, &stream, fixed_header, message_id) != SUCCES)
	{
		destroy_length(converted_length);
		return FAIL;
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
		return FAIL;
	}
#endif
	message_id = get_next_message_id(mqtt);

	length += MESSAGE_ID_SIZE;
	length += get_buffer_length(topic);
	length += QUALITY_OF_SERVICE_SIZE;


	if(convert_length(&converted_length, length))
	{
		return FAIL;
	}

	total_length = FIXED_HEADER_SIZE + converted_length->length + length;
	if(create_message(mqtt, result, total_length, &stream, fixed_header, message_id) != SUCCES)
	{
		destroy_length(converted_length);
		return FAIL;
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
	uint16_t packed_id,
	uint8_t return_code
	)
{
	struct pico_mqtt_data* stream = NULL;
	struct suback message = 
	{
		.fixed_header = SUBACK,
		.remaining_length = PACKET_ID_SIZE + RETURN_CODE_SIZE,
		.packed_id = packed_id
		.return_code = return_code
	};

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL) || ((return_code > 2) && (return_code != 0x80)))
	{
		return FAIL;
	}
#endif

	if(create_message(mqtt, result, sizeof(struct suback), &stream, message.fixed_header, packet_id) != SUCCES)
	{
		return FAIL;
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
	struct pico_mqtt_data* stream = NULL;
	uint8_t fixed_header = UNSUBSCRIBE | QUALITY_OF_SERVICE_1;
	struct pico_mqtt_data* converted_length = NULL;
	uint32_t length = PACKET_ID_SIZE;
	uint32_t total_length = 0;
	uint16_t message_id = 0;

#ifdef DEBUG /* this check should only be performed during debug */
	if((mqtt == NULL) || (result == NULL) || (topic == NULL))
	{
		return FAIL;
	}
#endif
	length += get_buffer_length(topic);
	message_id = get_message_id(mqtt);

	if(convert_length(&converted_length, length))
	{
		return FAIL;
	}

	total_length = FIXED_HEADER_SIZE + converted_length->length + length;
	if(create_message(mqtt, result, total_length, &stream, fixed_header, message_id) != SUCCES)
	{
		destroy_length(converted_length);
		return FAIL;
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
		return FAIL;
	}
#endif

	if(create_message(mqtt, result, sizeof(struct header), &stream, fixed_header, 0) != SUCCES)
	{
		return FAIL;
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
		return FAIL;
	}
#endif

	if(create_message(mqtt, result, sizeof(struct acknowledge), &stream, fixed_header, 0) != SUCCES)
	{
		return FAIL;
	}

	add_data_stream(&stream, (void*) &message, sizeof(struct acknowledge));
	return SUCCES;	
}

static int create_message(struct pico_mqtt* mqtt, struct pico_mqtt_message** result, size_t length, void** stream, uint8_t fixed_header, uint16_t message_id)
{
	*result = (struct pico_mqtt_message*) malloc(sizeof(struct pico_mqtt_message));
	*result = {
		.header = (struct pico_mqtt_fixed_header) fixed_header,
		.message_id = message_id,
		.topic = {.length = 0, .data = NULL},
		.data = {.length = 0, .data = NULL}};

	if(mqtt->malloc(mqtt, &((*result)->data.data), length) != SUCCES)
	{
		return FAIL;
	}
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
	struct pico_mqtt_data data;
	uint8_t i;
	uint32_t mask = 0x00200000;
	uint8_t continue_flag = 0;

	if(converted == NULL)
		return FAIL;

	if(length >= 0x10000000){
		return FAIL; /* //TODO set error to long */
	}

	data.length = 0;
	for(i = 3; i >=0; --i){
		if(length >= mask){
			if(data.length == 0){
				data.length = i+1;
				data.data = malloc(data.length); /* //TODO check if allocation worked */
				if(data.data == NULL)
					return FAIL;
			}
			
			*(((uint8_t*) data.data) + i) = length / mask;
			length = length % mask;
			if(continue_flag == 1)
				*(((uint8_t*) data.data) + i) |= 0x80;
			continue_flag = 1;
			mask >> 7;
		}
	}

	*converted = data;
	return 0;	
}

static inline void destroy_length(struct pico_mqtt_data* converted_length)
{
	free(converted_length->data);
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
	if(data != NULL)
	{
		add_data_stream(stream, (uint16_t*) &(data->length), LENGTH_SIZE);
		add_data_stream(stream, data->data, data->length);
	}
}

static void add_data_stream(void** stream, void* data, uint32_t length)
{
	int i = 0;
	for(i=0; i<length; ++i){
		**((uint8_t**)stream) = *((uint8_t*)data);
		(*((uint8_t**)stream))++;
		((uint8_t*)data)++;
	}
}
