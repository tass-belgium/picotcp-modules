#include "pico_mqtt.h"

/**
* Private Function Prototypes
**/

static struct pico_mqtt_data* terminated_string_to_data(const char* string);
static char* data_to_string(struct pico_mqtt_data* data);
static uint8_t is_valid_data( const struct pico_mqtt_data* data );
static uint8_t has_wildcards( const struct pico_mqtt_data* topic );
#if ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0
	static uint8_t has_only_alphanumeric( const struct pico_mqtt_data* data );
#endif
static uint8_t is_valid_unicode_string( const struct pico_mqtt_data* data);
#if ALLOW_SYSTEM_TOPICS == 0
	static uint8_t is_system_topic( const struct pico_mqtt_data* data);
#endif //#if ALLOW_SYSTEM_TOPICS == 0
static uint8_t is_valid_topic( struct pico_mqtt* mqtt, const struct pico_mqtt_data* topic);
static uint8_t is_quality_of_service_valid( struct pico_mqtt* mqtt, uint8_t quality_of_service);
static void set_error(struct pico_mqtt* mqtt, int error);
static int set_connect_options( struct pico_mqtt* mqtt );
static void set_new_packet_id( struct pico_mqtt* mqtt );
static int set_uri(struct pico_mqtt* mqtt, const char* uri_string);
static void unset_uri( struct pico_mqtt* mqtt);
static int set_port(struct pico_mqtt* mqtt, const char* port_string);
static void unset_port( struct pico_mqtt* mqtt);
static void destroy_packet(struct pico_mqtt_packet* packet);

static int protocol( struct pico_mqtt* mqtt, uint32_t timeout);
static int process_messages(struct pico_mqtt* mqtt);
static int receive_message( struct pico_mqtt* mqtt);
static void resend_messages(struct pico_mqtt* mqtt);
static void set_trigger_message( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet);
static uint8_t is_response_required( struct pico_mqtt_packet* packet );
static void check_if_request_complete(struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet);
static int send_acknowledge( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet, uint8_t update_trigger_flag);
static uint8_t is_trigger_message( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet );
static int handle_error_message( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet );
static int handle_connack( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet );
static int handle_publish( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet );
static int handle_pubrec( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet );
static int handle_pubrel( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet );
static int handle_acknowledge( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet );
static int handle_pingreq( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet );
static int handle_pingresp( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet );

/**
* MQTT status
**/

#define INITIALIZE 0
#define NOT_CONNECTED 1
#define CONNECTED 2

/**
* Public Function Implementation
**/

struct pico_mqtt* pico_mqtt_create( void )
{
	struct pico_mqtt* mqtt = (struct pico_mqtt*) MALLOC (sizeof(struct pico_mqtt));
	if(mqtt == NULL)
		return NULL;

	*mqtt = PICO_MQTT_EMPTY;

	mqtt->stream = pico_mqtt_stream_create( &(mqtt->error) );
	mqtt->serializer = pico_mqtt_serializer_create( &(mqtt->error));
	mqtt->output_queue = pico_mqtt_list_create( &(mqtt->error) );
	mqtt->wait_queue = pico_mqtt_list_create( &(mqtt->error) );
	mqtt->input_queue = pico_mqtt_list_create( &(mqtt->error) );
	if(	mqtt->stream == NULL ||
		mqtt->serializer == NULL ||
		mqtt->output_queue == NULL||
		mqtt->wait_queue == NULL||
		mqtt->input_queue == NULL )
	{
		pico_mqtt_destroy( mqtt );
		return NULL;
	}

	return mqtt;
}

void pico_mqtt_destroy(struct pico_mqtt* mqtt)
{	
	if(mqtt == NULL)
		return;

	pico_mqtt_stream_destroy(mqtt->stream);
	pico_mqtt_serializer_destroy(mqtt->serializer);
	pico_mqtt_list_destroy( mqtt->output_queue );
	pico_mqtt_list_destroy( mqtt->wait_queue );
	pico_mqtt_list_destroy( mqtt->input_queue );
	pico_mqtt_destroy_data( mqtt->client_id );
	pico_mqtt_destroy_data( mqtt->username );
	pico_mqtt_destroy_data( mqtt->password );
	pico_mqtt_destroy_data( mqtt->uri );
	pico_mqtt_destroy_data( mqtt->port );
	pico_mqtt_destroy_message( mqtt->will_message );

	FREE(mqtt);
}

int pico_mqtt_set_client_id(struct pico_mqtt* mqtt, const char* client_id_string)
{
	struct pico_mqtt_data* client_id = NULL;
	if(mqtt == NULL)
	{
		PERROR("The mqtt object is not set\n");
		return ERROR;
	}

#if ALLOW_EMPTY_CLIENT_ID == 0
	if(client_id_string == NULL)
	{
		PTODO("set the appropriate error.\n");
		PERROR("An empty client ID is not allowed. (ALLOW_EMPTY_CLIENT_ID = 0)\n");
		return ERROR;
	}
#endif

	client_id = pico_mqtt_string_to_data( client_id_string );
	if(client_id == NULL)
	{
		PTODO("set the appropriate error.\n");
		PERROR("Unable to allocate the memory needed.\n");
		return ERROR;
	}

#if ALLOW_LONG_CLIENT_ID == 0
	if(client_id->length > 23)
	{
		pico_mqtt_destroy_data(client_id);
		PTODO("set the appropriate error.\n");
		PERROR("The client_id may not be longer than 23 bytes. (ALLOW_LONG_CLIENT_ID = 0)\n");
		return ERROR;
	}
#endif

#if ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0
	if(!has_only_alphanumeric( client_id ))
	{
		pico_mqtt_destroy_data(client_id);
		PTODO("set the appropriate error.\n");
		PERROR("The client_id may only contain alpha numeric characters. (ALLOW_NON_ALPHNUMERIC_CLIENT_ID = 0)\n");
		return ERROR;
	}
#else
	if(!is_valid_unicode_string( client_id ))
	{
		pico_mqtt_destroy_data(client_id);
		PTODO("set the appropriate error.\n");
		PERROR("The client id may not contain control characters\n");
		return ERROR;
	}
#endif

	pico_mqtt_unset_client_id( mqtt );
	mqtt->client_id = client_id;
	return SUCCES;
}

void pico_mqtt_unset_client_id( struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PWARNING("Unsetting a field without passing valid mqtt object has no effect.\n");
		return;
	}

	if(mqtt->client_id != NULL)
		pico_mqtt_destroy_data(mqtt->client_id);

	mqtt->client_id = NULL;
}

int pico_mqtt_set_will_message(struct pico_mqtt* mqtt, const struct pico_mqtt_message* will_message)
{
	if((mqtt == NULL) || (will_message == NULL))
	{
		PTODO("set the appropriate error.\n");
		PERROR("Not all parameters are set correctly\n");
		return ERROR;
	}

	if(!is_valid_topic(mqtt, will_message->topic))
	{
		PTRACE();
		return ERROR;
	}

	if(has_wildcards(will_message->topic))
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The topic may not contain wildcards.\n");
		return ERROR;
	}

	pico_mqtt_unset_will_message(mqtt);
	mqtt->will_message = pico_mqtt_copy_message(will_message);

	return SUCCES;
}

void pico_mqtt_unset_will_message( struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PWARNING("Unsetting a field without passing valid mqtt object has no effect.\n");
		return;
	}

	if(mqtt->will_message != NULL)
		pico_mqtt_destroy_message(mqtt->will_message);

	mqtt->will_message = NULL;
}

int pico_mqtt_set_username(struct pico_mqtt* mqtt, const char* username_string)
{
	struct pico_mqtt_data* username = NULL;
	if(mqtt == NULL)
	{
		PERROR("The mqtt object is not set\n");
		return ERROR;
	}

#if ALLOW_EMPTY_USERNAME == 0
	if(username_string == NULL)
	{
		PTODO("set the appropriate error.\n");
		PERROR("An empty username is not allowed. (ALLOW_EMPTY_USERNAME = 0)\n");
		return ERROR;
	}
#endif

	username = pico_mqtt_string_to_data( username_string );
	if(username == NULL)
	{
		PTODO("set the appropriate error.\n");
		PERROR("Unable to allocate the memory needed.\n");
		return ERROR;
	}

	if(!is_valid_unicode_string(username))
	{
		pico_mqtt_destroy_data(username);
		PTODO("set the appropriate error.\n");
		PERROR("The username contains illegal characters\n");
		return ERROR;
	}

	pico_mqtt_unset_username( mqtt );

	mqtt->username = username;
	return SUCCES;
}

void pico_mqtt_unset_username( struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PWARNING("Unsetting a field without passing valid mqtt object has no effect.\n");
		return;
	}

	if(mqtt->username != NULL)
		pico_mqtt_destroy_data(mqtt->username);

	mqtt->username = NULL;
}

int pico_mqtt_set_password(struct pico_mqtt* mqtt, const char* password_string)
{
	struct pico_mqtt_data* password = NULL;
	if(mqtt == NULL)
	{
		PERROR("The mqtt object is not set\n");
		return ERROR;
	}

	password = pico_mqtt_string_to_data( password_string );
	if(password == NULL)
	{
		PTODO("set the appropriate error.\n");
		PERROR("Unable to allocate the memory needed.\n");
		return ERROR;
	}

	pico_mqtt_unset_password( mqtt );

	mqtt->password = password;
	return SUCCES;
}

void pico_mqtt_unset_password( struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PWARNING("Unsetting a field without passing valid mqtt object has no effect.\n");
		return;
	}

	if(mqtt->password != NULL)
		pico_mqtt_destroy_data(mqtt->password);

	mqtt->password = NULL;
}

void pico_mqtt_set_keep_alive_time(struct pico_mqtt* mqtt, const uint16_t keep_alive_time)
{
	if(mqtt == NULL)
	{
		PWARNING("Unsetting a field without passing valid mqtt object has no effect.\n");
		return;
	}

	mqtt->keep_alive_time = keep_alive_time;
}

int pico_mqtt_connect(struct pico_mqtt* mqtt, const char* uri, const char* port, const uint32_t timeout)
{
	struct pico_mqtt_packet* packet = NULL;
	if(mqtt == NULL)
	{
		PWARNING("The MQTT object is not specified, no action will be done.\n");
		return ERROR;
	}

	if(mqtt->connect_send != 0)
	{
		PWARNING("Attempting to call the connect function more than once can lead to unexpected results.\n");
	}

	if(set_uri(mqtt, uri) == ERROR)
		return ERROR;

	if(set_port(mqtt, port) == ERROR)
		return ERROR;

	if(set_connect_options(mqtt) == ERROR)
		return ERROR;

	if(pico_mqtt_stream_connect( mqtt->stream, uri, port) == ERROR )
		return ERROR;

	if(pico_mqtt_serialize(mqtt->serializer) == ERROR)
		return ERROR;

	packet = pico_mqtt_serializer_get_packet(mqtt->serializer);

	pico_mqtt_list_push_back(mqtt->output_queue, packet);

	mqtt->connect_send = 1;

	set_trigger_message(mqtt, packet);
	if(protocol(mqtt, timeout) == ERROR)
		return ERROR;

	return SUCCES;
}

int pico_mqtt_disconnect(struct pico_mqtt* mqtt)
{
	struct pico_mqtt_packet* packet = NULL;
	if(mqtt == NULL)
	{
		PWARNING("The MQTT object is not specified, no action will be done.\n");
		return ERROR;
	}

	pico_mqtt_serializer_set_message_type(mqtt->serializer, DISCONNECT);

	if(pico_mqtt_serialize(mqtt->serializer) == ERROR)
		return ERROR;

	packet = pico_mqtt_serializer_get_packet(mqtt->serializer);

	pico_mqtt_list_push_back(mqtt->output_queue, packet);

	set_trigger_message(mqtt, packet);
	if(protocol(mqtt, 1) == ERROR)
		return ERROR;

	return SUCCES;
}

int pico_mqtt_ping(struct pico_mqtt* mqtt, const uint32_t timeout )
{
	struct pico_mqtt_packet* packet = NULL;
	if(mqtt == NULL)
	{
		PWARNING("The MQTT object is not specified, no action will be done.\n");
		return ERROR;
	}

	pico_mqtt_serializer_set_message_type(mqtt->serializer, PINGREQ);

	if(pico_mqtt_serialize(mqtt->serializer) == ERROR)
		return ERROR;

	packet = pico_mqtt_serializer_get_packet(mqtt->serializer);

	pico_mqtt_list_push_back(mqtt->output_queue, packet);

	set_trigger_message(mqtt, NULL);
	if(protocol(mqtt, timeout) == ERROR)
		return ERROR;

	return SUCCES;
}

int pico_mqtt_publish(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, const uint32_t timeout)
{
	struct pico_mqtt_packet* packet = NULL;

	if(mqtt == NULL)
	{
		PWARNING("The MQTT object is not specified, no action will be done.\n");
		return ERROR;
	}

	if(!is_valid_topic(mqtt, message->topic))
	{
		PTRACE();
		return ERROR;
	}

	if(has_wildcards(message->topic))
	{
		PTODO("Set the appropriate error.\n");
		PERROR("The topic may not contain wildcards.\n");
		return ERROR;
	}

	if(!is_quality_of_service_valid(mqtt, message->quality_of_service))
		return ERROR;

	pico_mqtt_serializer_set_message_type(mqtt->serializer, PUBLISH);
	pico_mqtt_serializer_set_message(mqtt->serializer, message);

	if(message->quality_of_service > 0)
		set_new_packet_id(mqtt);

	if(pico_mqtt_serialize(mqtt->serializer) == ERROR)
		return ERROR;

	packet = pico_mqtt_serializer_get_packet(mqtt->serializer);
	packet->message = NULL;

	pico_mqtt_list_push_back(mqtt->output_queue, packet);

	set_trigger_message(mqtt, packet);
	if(protocol(mqtt, timeout) == ERROR)
		return ERROR;

	return SUCCES;
}

int pico_mqtt_receive(struct pico_mqtt* mqtt, struct pico_mqtt_message** message, const uint32_t timeout)
{
	if(mqtt == NULL)
	{
		PWARNING("The MQTT object is not specified, no action will be done.\n");
		return ERROR;
	}

	if(message == NULL)
	{
		PWARNING("The message object is not specified, no action will be done.\n");
		return ERROR;
	}

	*message = NULL;

	set_trigger_message(mqtt, NULL);
	if(protocol(mqtt, timeout) == ERROR)
		return ERROR;

	if(pico_mqtt_list_length(mqtt->input_queue) > 0)
	{
		struct pico_mqtt_packet* packet = pico_mqtt_list_pop(mqtt->input_queue);
		*message = packet->message;
		packet->message = NULL;
		destroy_packet(packet);
	}

	return SUCCES;
}

int pico_mqtt_subscribe(struct pico_mqtt* mqtt, const char* topic_string, const uint8_t quality_of_service, const uint32_t timeout)
{
	struct pico_mqtt_packet* packet = NULL;
	struct pico_mqtt_data* topic = NULL;

	if(mqtt == NULL)
	{
		PWARNING("The MQTT object is not specified, no action will be done.\n");
		return ERROR;
	}

	if(!is_quality_of_service_valid(mqtt, quality_of_service))
		return ERROR;

	topic = pico_mqtt_string_to_data(topic_string);
	if(topic == NULL)
	{
		PERROR("Unable to allocate the memory needed.\n");;
		PTODO("set the appropriate error.\n");
		return ERROR;
	}

	if(!is_valid_topic(mqtt, topic))
	{
		PTRACE();
		pico_mqtt_destroy_data(topic);
		return ERROR;
	}

	pico_mqtt_serializer_set_message_type(mqtt->serializer, SUBSCRIBE);
	pico_mqtt_serializer_set_topic(mqtt->serializer, topic);
	pico_mqtt_serializer_set_quality_of_service(mqtt->serializer, quality_of_service);
	set_new_packet_id(mqtt);

	if(pico_mqtt_serialize(mqtt->serializer) == ERROR)
	{
		pico_mqtt_destroy_data(topic);
		pico_mqtt_serializer_clear(mqtt->serializer);
		return ERROR;
	}

	packet = pico_mqtt_serializer_get_packet(mqtt->serializer);

	pico_mqtt_list_push_back(mqtt->output_queue, packet);

	set_trigger_message(mqtt, packet);
	if(protocol(mqtt, timeout) == ERROR)
		return ERROR;

	return SUCCES;
}

int pico_mqtt_unsubscribe(struct pico_mqtt* mqtt, const char* topic_string, const uint32_t timeout)
{
	struct pico_mqtt_packet* packet = NULL;
	struct pico_mqtt_data* topic = NULL;

	if(mqtt == NULL)
	{
		PWARNING("The MQTT object is not specified, no action will be done.\n");
		return ERROR;
	}

	topic = pico_mqtt_string_to_data(topic_string);
	if(topic == NULL)
	{
		PERROR("Unable to allocate the memory needed.\n");;
		PTODO("set the appropriate error.\n");
		return ERROR;
	}

	if(!is_valid_topic(mqtt, topic))
	{
		PTRACE();
		pico_mqtt_destroy_data(topic);
		return ERROR;
	}

	pico_mqtt_serializer_set_message_type(mqtt->serializer, UNSUBSCRIBE);
	pico_mqtt_serializer_set_topic(mqtt->serializer, topic);
	set_new_packet_id(mqtt);

	if(pico_mqtt_serialize(mqtt->serializer) == ERROR)
	{
		pico_mqtt_destroy_data(topic);
		return ERROR;
	}

	//pico_mqtt_destroy_data(topic);

	packet = pico_mqtt_serializer_get_packet(mqtt->serializer);

	pico_mqtt_list_push_back(mqtt->output_queue, packet);

	set_trigger_message(mqtt, packet);
	if(protocol(mqtt, timeout) == ERROR)
		return ERROR;

	return SUCCES;
}

/**
* Helper Function Implementation
**/

struct pico_mqtt_data* pico_mqtt_string_to_data(const char* string)
{
	uint32_t length = 0;
	const char* ptr = string;

	if(string == NULL)
		return pico_mqtt_create_data(NULL, 0);

	while(*ptr != '\0')
	{
		++ptr;
		++length;
	}

	return pico_mqtt_create_data(string, length);
}

struct pico_mqtt_message* pico_mqtt_create_message(const char* topic, const void* data, const uint32_t length)
{
	struct pico_mqtt_message* message = (struct pico_mqtt_message*) MALLOC(sizeof(struct pico_mqtt_message));
	if(message == NULL)
	{
		PERROR("Failed to allocate the required memory.\n");
		return NULL;
	}

	*message = PICO_MQTT_MESSAGE_EMPTY;
	message->topic = pico_mqtt_string_to_data(topic);
	message->data = pico_mqtt_create_data(data, length);

	if((message->topic == NULL) ||
		(message->data == NULL))
	{
		pico_mqtt_destroy_data(message->topic);
		pico_mqtt_destroy_data(message->data);
	}

	return message;
}

struct pico_mqtt_message* pico_mqtt_copy_message( const struct pico_mqtt_message* message)
{
	struct pico_mqtt_message* copy = NULL;
	if(message == NULL)
		return NULL;

	copy = (struct pico_mqtt_message*) MALLOC(sizeof(struct pico_mqtt_message));
	if(copy == NULL)
	{
		PERROR("Failed to allocate the required memory.\n");
		return NULL;
	}

	*copy = *message;
	copy->topic = pico_mqtt_copy_data(message->topic);
	copy->data = pico_mqtt_copy_data(message->data);

	if((copy->data == NULL) || (copy->topic == NULL))
	{
		pico_mqtt_destroy_message(copy);
		PERROR("Failed to allocate the required memory.\n");
		return NULL;
	}

	return copy;
}

void pico_mqtt_destroy_message(struct pico_mqtt_message * message)
{
	if(message == NULL)
		return;

	pico_mqtt_destroy_data(message->topic);
	pico_mqtt_destroy_data(message->data);
	FREE(message);
}

void pico_mqtt_destroy_data(struct pico_mqtt_data * data)
{
	if(data == NULL)
		return;

	FREE(data->data);
	FREE(data);
}

struct pico_mqtt_data* pico_mqtt_create_data(const void* data, const uint32_t length)
{
	struct pico_mqtt_data* result = (struct pico_mqtt_data*) MALLOC(sizeof(struct pico_mqtt_data));
	uint32_t i = 0;

	if(result == NULL)
	{
		PERROR("Failed to allocate the required memory.\n");
		return NULL;
	}

	result->length = length;
	if(data == NULL)
	{
		result->data = NULL;
		return result;
	}

	result->data = MALLOC(length);
	if(result->data == NULL)
	{
		PERROR("Failed to allocate the required memory.\n");
		FREE(result);
		return NULL;
	}
	
	for(i=0; i < length; ++i)
	{
		*(((uint8_t*) result->data) + i)=*(((const uint8_t*) data) + i);
	}
	
	return result;
}

struct pico_mqtt_data* pico_mqtt_copy_data( const struct pico_mqtt_data* data)
{
	if(data == NULL)
		return NULL;

	return pico_mqtt_create_data(data->data, data->length);
}

void pico_mqtt_message_set_quality_of_service(struct pico_mqtt_message* message, uint8_t quality_of_service)
{
	CHECK_NOT_NULL(message);
	if(!is_quality_of_service_valid(NULL, quality_of_service))
		return;

	message->quality_of_service = quality_of_service;
}

uint8_t pico_mqtt_message_get_quality_of_service(struct pico_mqtt_message* message)
{
	CHECK_NOT_NULL(message);
	return message->quality_of_service;
}

void pico_mqtt_message_set_retain(struct pico_mqtt_message* message, uint8_t retain_flag )
{
	CHECK_NOT_NULL(message);
	if(retain_flag > 1)
	{
		PERROR("The retain_flag is out of specifications, only 0 or 1 is allowed.");
		return;
	}

	message->retain = (uint8_t)retain_flag;
}

uint8_t pico_mqtt_message_get_retain(struct pico_mqtt_message* message)
{
	CHECK_NOT_NULL(message);
	return message->retain;
}

uint8_t pico_mqtt_message_is_duplicate_flag_set(struct pico_mqtt_message* message)
{
	CHECK_NOT_NULL(message);
	return message->duplicate;
}

uint16_t pico_mqtt_message_get_message_id(struct pico_mqtt_message* message)
{
	CHECK_NOT_NULL(message);
	return message->message_id;
}

char* pico_mqtt_message_get_topic(struct pico_mqtt_message* message)
{
	char* string = NULL;
	uint32_t i = 0;
	CHECK_NOT_NULL(message);

	if(message->topic == NULL)
		return NULL;

	if(message->topic->length == 0)
		return NULL;

	CHECK_NOT_NULL(message->topic->data);

	string = (char*) MALLOC(message->topic->length + 1);
	if(string == NULL)
	{
		PERROR("Failed to allocate the required memory.\n");
		return NULL;
	}

	for(i=0; i<message->topic->length; ++i)
	{
		*(string + i) = *(((char*)message->topic->data) + i);
	}

	*(string + message->topic->length) = '\0';

	return string;
}

const char* pico_mqtt_get_protocol_version( void )
{
	return "3.1.1";
}

/**
* Private Function Implemenation
**/


static struct pico_mqtt_data* terminated_string_to_data(const char* string)
{
	uint32_t length = 0;
	char termination = '\0';
	const char* ptr = string;

	if(string == NULL)
		return pico_mqtt_create_data(&termination, 1);

	while(*ptr != '\0')
	{
		++ptr;
		++length;
	}

	return pico_mqtt_create_data(string, length + 1);
}

static char* data_to_string(struct pico_mqtt_data* data)
{
	CHECK_NOT_NULL(data);
	return (char*) data->data;
}

static uint8_t is_valid_data( const struct pico_mqtt_data* data )
{
	if(data == NULL)
		return 1;

	return (data->length == 0) == (data->data == NULL);
}

static uint8_t has_wildcards( const struct pico_mqtt_data* topic )
{
	uint32_t i = 0;
	char byte = 0;

	CHECK_NOT_NULL(topic);
	CHECK(is_valid_data(topic), "The data object is invallid, check before use.\n");

	if(topic->length == 0)
		return 0;

	for(i=0; i<topic->length; ++i)
	{
		byte = *(((char*)topic->data) + i);
		if((byte == '#') || (byte == '+'))
			return 1;
	}

	return 0;
}

#if ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0
static uint8_t has_only_alphanumeric( const struct pico_mqtt_data* data )
{
	uint32_t i = 0;
	char byte = 0;

	CHECK_NOT_NULL(data);
	CHECK(is_valid_data(data), "The data object is invallid, check before use.\n");

	if(data->length == 0)
		return 1;

	for(i=0; i<data->length; ++i)
	{
		byte = *(((char*)data->data) + i);
		if(	(byte < '0') || 
			((byte > '9') && (byte < 'A')) ||
			((byte > 'Z') && (byte < 'a')) ||
			(byte > 'z'))
			return 0;
	}

	return 1;
}
#endif // ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0

static uint8_t is_valid_unicode_string( const struct pico_mqtt_data* data)
{
	uint32_t i = 0;
	uint8_t byte = 0;

	CHECK_NOT_NULL(data);
	CHECK(is_valid_data(data), "The data object is invallid, check before use.\n");

	if(data->length == 0)
		return 1;

	for(i=0; i<data->length; ++i)
	{
		byte = *(((uint8_t*)data->data) + i);
		if(	(byte < 0x20) || ((byte > 0x7E) && (byte < 0xA0)))
			return 0;
	}

	return 1;
}

#if ALLOW_SYSTEM_TOPICS == 0
static uint8_t is_system_topic( const struct pico_mqtt_data* data)
{
	CHECK_NOT_NULL(data);
	CHECK(is_valid_data(data), "The data object is invallid, check before use.\n");

	if(data->length == 0)
		return 0;

	if(*((char*)data->data) == '$')
		return 1;

	return 0;
}
#endif //#if ALLOW_SYSTEM_TOPICS == 0

static uint8_t is_valid_topic( struct pico_mqtt* mqtt, const struct pico_mqtt_data* topic)
{
	if(topic == NULL)
	{
		PTODO("set the appropriate error.\n");
		set_error(mqtt, ERROR);
		PERROR("The topic must be set\n");
		return 0;
	}

	if(topic->length == 0)
	{
		PTODO("Set the appropriate error.\n");
		set_error(mqtt, ERROR);
		PERROR("The topic may not be empty.\n");
		return 0;
	}

#if ALLOW_SYSTEM_TOPICS == 0
	if(is_system_topic(topic))
	{
		PTODO("Set the appropriate error.\n");
		set_error(mqtt, ERROR);
		PERROR("The topic may not be a system topic (start with $).\n");
		return 0;
	}
#endif //#if ALLOW_SYSTEM_TOPICS == 0

	if(!is_valid_unicode_string(topic))
	{
		PTODO("set the appropriate error.\n");
		set_error(mqtt, ERROR);
		PERROR("The topic contains illegal characters\n");
		return 0;
	}

	return 1;
}

static uint8_t is_quality_of_service_valid( struct pico_mqtt* mqtt, uint8_t quality_of_service)
{
	if(quality_of_service > 2)
	{
		PTODO("Set an appropriate error\n");
		set_error(mqtt, ERROR);
		PERROR("The quality_of_service is out of specifications, only 0, 1 or 2 is allowed.");
		return 0;
	}

	return 1;
}

static void set_error(struct pico_mqtt* mqtt, int error)
{
	if(mqtt != NULL)
		mqtt->error = error;
}

static int set_connect_options( struct pico_mqtt* mqtt )
{
	CHECK_NOT_NULL(mqtt);

	pico_mqtt_serializer_set_message_type( mqtt->serializer, CONNECT );
	
#if ALLOW_EMPTY_CLIENT_ID == 0
	if(mqtt->client_id == NULL)
	{
		PERROR("The client_id must be set and is not, ALLOW_EMPTY_CLIENT_ID == 0.\n");
		PTODO("set the appropriate error\n");
		return ERROR;
	}		
#else
	if(mqtt->client_id == NULL)
	{
		pico_mqtt_set_client_id(mqtt, NULL);
	}
#endif //ALLOW_EMPTY_CLIENT_ID == 0
	
	pico_mqtt_serializer_set_client_id( mqtt->serializer, mqtt->client_id);

#if REQUIRE_USERNAME != 0
#if ALLOW_EMPTY_USERNAME == 0
	if(mqtt->username == NULL)
	{
		PERROR("The username must be set to a non-empty string and is not, REQUIRE_USERNAME ==1, ALLOW_EMPTY_USERNAME == 0.\n");
		PTODO("set the appropriate error\n");
		return ERROR;
	}		
#else
	if(mqtt->username == NULL)
	{
		PWARNING("Username is required but not specified, using empty username.\n");
		pico_mqtt_set_username(mqtt, NULL);
	}
#endif //ALLOW_EMPTY_USERNAME == 0
#endif //REQUIRE_USERNAME != 0

	if(mqtt->username != NULL)
	{
		pico_mqtt_serializer_set_username( mqtt->serializer, mqtt->username );
	} else
	{
		if(mqtt->password != NULL)
		{
			PERROR("The username is not set but the password is.\n");
			return ERROR;
		}
	}

#if REQUIRE_PASSWORD != 0
	if(mqtt->password == NULL)
	{
		PWARNING("Password is required but not specified, using empty password.\n");
		pico_mqtt_set_password(mqtt, NULL);
	}
#endif //REQUIRE_PASSWORD != 0

	if(mqtt->password != NULL)
		pico_mqtt_serializer_set_password( mqtt->serializer, mqtt->password);


	if(mqtt->will_message != NULL)
		pico_mqtt_serializer_set_message( mqtt->serializer, mqtt->will_message);

	pico_mqtt_serializer_set_keep_alive_time( mqtt->serializer, mqtt->keep_alive_time);

	return SUCCES;
}

static void set_new_packet_id( struct pico_mqtt* mqtt )
{
	CHECK_NOT_NULL(mqtt);
	CHECK_NOT_NULL(mqtt->serializer);

	pico_mqtt_serializer_set_message_id(mqtt->serializer, mqtt->next_message_id);
	mqtt->next_message_id++;
}

static int set_uri(struct pico_mqtt* mqtt, const char* uri_string)
{
	struct pico_mqtt_data* uri;
	if(mqtt == NULL)
	{
		PTODO("set the appropriate error.\n");
		PERROR("Not all parameters are set correctly\n");
		return ERROR;
	}

	uri = terminated_string_to_data( uri_string );
	if(uri == NULL)
	{
		PTODO("set the appropriate error.\n");
		PERROR("Unable to allocate the memory needed.\n");
		return ERROR;
	}

	unset_uri(mqtt);
	mqtt->uri = uri;

	return SUCCES;
}

static void unset_uri( struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PWARNING("Unsetting a field without passing valid mqtt object has no effect.\n");
		return;
	}

	if(mqtt->uri != NULL)
		pico_mqtt_destroy_data(mqtt->uri);

	mqtt->uri = NULL;
}

static int set_port(struct pico_mqtt* mqtt, const char* port_string)
{
	struct pico_mqtt_data* port;
	if(mqtt == NULL)
	{
		PTODO("set the appropriate error.\n");
		PERROR("Not all parameters are set correctly\n");
		return ERROR;
	}

	port = terminated_string_to_data( port_string );
	if(port == NULL)
	{
		PTODO("set the appropriate error.\n");
		PERROR("Unable to allocate the memory needed.\n");
		return ERROR;
	}

	unset_port(mqtt);
	mqtt->port = port;

	return SUCCES;
}

static void unset_port( struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PWARNING("Unsetting a field without passing valid mqtt object has no effect.\n");
		return;
	}

	if(mqtt->port != NULL)
		pico_mqtt_destroy_data(mqtt->port);

	mqtt->port = NULL;
}

static void destroy_packet(struct pico_mqtt_packet* packet)
{
	if(packet == NULL)
		return;

	pico_mqtt_destroy_message(packet->message);
	FREE(packet->streamed.data);
	FREE(packet);
}


/**
* The protocol state machine
**/

static int protocol( struct pico_mqtt* mqtt, uint32_t timeout)
{
	uint64_t time_left = timeout;
	CHECK_NOT_NULL(mqtt);

	while((time_left != 0) && (!mqtt->request_complete))
	{
		if(mqtt->status == INITIALIZE)
		{
			if(pico_mqtt_stream_connect( mqtt->stream, data_to_string(mqtt->uri), data_to_string(mqtt->port)) == SUCCES)
			{
				mqtt->status = CONNECTED;
			} else
			{
				return ERROR;
			}
		}

		if(mqtt->status == NOT_CONNECTED)
		{
			// SET the clean session flag to 0!!!!!!!!!!!
			if(pico_mqtt_stream_connect( mqtt->stream, data_to_string(mqtt->uri), data_to_string(mqtt->port)) == SUCCES)
			{
				mqtt->status = CONNECTED;
				mqtt->connection_attempts = 0;
			} else
			{
				mqtt->connection_attempts++;

				if((MAXIMUM_RECONNECTION_ATTEMPTS != 0) && (mqtt->connection_attempts > MAXIMUM_RECONNECTION_ATTEMPTS))
				{
					PERROR("Maximum nummber of connection_attempts reached MAXIMUM_RECONNECTION_ATTEMPTS == %d\n", MAXIMUM_RECONNECTION_ATTEMPTS );
					PTODO("Set the appropriate error.\n");
					return ERROR;
				}
			}
		}

		if(mqtt->status == CONNECTED)
		{
			if(process_messages(mqtt) == ERROR)
			{
				PERROR("An error occured while communicating with the broker.\n");
				pico_mqtt_stream_destroy(mqtt->stream);
				pico_mqtt_stream_create(&(mqtt->error));
				resend_messages(mqtt);
				mqtt->status = NOT_CONNECTED;
			}

			if(mqtt->request_complete)
				return SUCCES;

			if(pico_mqtt_stream_send_receive( mqtt->stream, &time_left, 
				(mqtt->trigger_message == NULL) || (is_response_required(mqtt->trigger_message))) == ERROR)
				return ERROR;
		}
	}

	return SUCCES;
}

static int process_messages(struct pico_mqtt* mqtt)
{
	if(!pico_mqtt_stream_is_message_sending(mqtt->stream))
	{
		if(mqtt->outgoing_packet != NULL)
		{
			if(is_response_required(mqtt->outgoing_packet))
			{
				if(pico_mqtt_list_push_back(mqtt->wait_queue, mqtt->outgoing_packet) == ERROR)
					return ERROR;

				mqtt->outgoing_packet = NULL;
			} else {
				check_if_request_complete(mqtt, mqtt->outgoing_packet);
				destroy_packet(mqtt->outgoing_packet);
				mqtt->outgoing_packet = NULL;
			}
		}

		if(pico_mqtt_list_length(mqtt->output_queue) != 0)
		{
			mqtt->outgoing_packet = pico_mqtt_list_pop(mqtt->output_queue);
			pico_mqtt_stream_set_send_message(mqtt->stream, mqtt->outgoing_packet->streamed);
		}
	}

	if(receive_message(mqtt) == ERROR)
		return ERROR;

	if(mqtt->trigger_message == NULL)
	{
		if(pico_mqtt_list_length(mqtt->input_queue) > 0)
			mqtt->request_complete = 1;
	}

	return SUCCES;
}

static int receive_message( struct pico_mqtt* mqtt )
{
	int(* const handlers[16])( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet ) =
	{
		handle_error_message,	// 00 - RESERVED    
		handle_error_message,	// 01 - CONNECT     
		handle_connack,			// 02 - CONNACK     
		handle_publish,			// 03 - PUBLISH     
		handle_acknowledge,		// 04 - PUBACK      
		handle_pubrec,			// 05 - PUBREC      
		handle_pubrel,			// 06 - PUBREL      
		handle_acknowledge,		// 07 - PUBCOMP     
		handle_error_message,	// 08 - SUBSCRIBE	   
		handle_acknowledge,		// 09 - SUBACK      
		handle_error_message,	// 10 - UNSUBSCRIBE 
		handle_acknowledge,		// 11 - UNSUBACK    
		handle_pingreq,			// 12 - PINGREQ     
		handle_pingresp,		// 13 - PINGRESP    
		handle_error_message,	// 14 - DISCONNECT  
		handle_error_message,	// 15 - RESERVED    
	};

	CHECK_NOT_NULL(mqtt);

	if(pico_mqtt_stream_is_message_received(mqtt->stream))
	{
		struct pico_mqtt_packet* packet = NULL;
		struct pico_mqtt_data data = pico_mqtt_stream_get_received_message(mqtt->stream);
		pico_mqtt_serializer_clear(mqtt->serializer);

		if(pico_mqtt_deserialize(mqtt->serializer, data) == ERROR)
			return ERROR;

		packet = pico_mqtt_serializer_get_packet(mqtt->serializer);
		if(packet == NULL)
		{
			PERROR("unable to get deserialized packet, not enough memory.\n");
			PTODO("set the appropriate error.\n");
			return ERROR;
		}

		if(handlers[packet->type](mqtt, packet) == ERROR)
			return ERROR;
	}

	return SUCCES;
}

static void resend_messages(struct pico_mqtt* mqtt)
{
	CHECK_NOT_NULL(mqtt);
	CHECK_NOT_NULL(mqtt->stream);

	pico_mqtt_list_push_back(mqtt->output_queue, mqtt->outgoing_packet);

	while(pico_mqtt_list_length(mqtt->wait_queue) > 0)
	{
		struct pico_mqtt_packet* packet = pico_mqtt_list_pop(mqtt->wait_queue);
		pico_mqtt_list_push_back(mqtt->output_queue, packet);
	}
}

static void set_trigger_message( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet)
{
	CHECK_NOT_NULL(mqtt);
	mqtt->request_complete = 0;
	mqtt->trigger_message = packet;
}

static uint8_t is_response_required( struct pico_mqtt_packet* packet )
{
	if(
		((packet->type == PUBLISH) && (packet->qos > 0)) ||
		(packet->type == PUBREC) ||
		(packet->type == PUBREL) ||
		(packet->type == SUBSCRIBE) ||
		(packet->type == UNSUBSCRIBE)
	)
		return 1;

	return 0;
}

static void check_if_request_complete(struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet)
{
	CHECK_NOT_NULL(mqtt);

	if(packet == NULL)
		return;

	if(mqtt->trigger_message == packet)
	{
		mqtt->request_complete = 1;
		mqtt->trigger_message = NULL;
	}
}

static int send_acknowledge( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet, uint8_t update_trigger_flag)
{
	struct pico_mqtt_packet* response_packet = NULL;

	CHECK_NOT_NULL(mqtt);
	CHECK(packet->type < 15, "Invalli message type.\n");
	
	pico_mqtt_serializer_clear(mqtt->serializer);
	pico_mqtt_serializer_set_message_type(mqtt->serializer, (uint8_t) (packet->type + 1));
	pico_mqtt_serializer_set_message_id(mqtt->serializer, (uint16_t) packet->packet_id);
	if(pico_mqtt_serialize(mqtt->serializer) == ERROR)
		return ERROR;

	if(pico_mqtt_list_push_back(mqtt->output_queue, response_packet) == ERROR)
		return ERROR;

	if(update_trigger_flag && (mqtt->trigger_message != NULL))
		mqtt->trigger_message = response_packet;

	response_packet->message = packet->message;
	packet->message = NULL;

	return SUCCES;
}

static uint8_t is_trigger_message( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet )
{
	return (packet == mqtt->trigger_message) && (packet != NULL);
}


static int handle_error_message( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet )
{
	CHECK_NOT_NULL(mqtt);
	destroy_packet(packet);
	PERROR("The message type may not be send to a client, disconnecting.\n");
	PTODO("set the appropriate error\n");
	return ERROR;
}

static int handle_connack( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet )
{
	CHECK_NOT_NULL(mqtt);
	CHECK(packet->type == CONNACK, "Wrong handler for message.\n");
	destroy_packet(packet);
	return SUCCES;
}

static int handle_publish( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet )
{
	CHECK_NOT_NULL(mqtt);
	CHECK(packet->type == PUBLISH, "Wrong handler for message.\n");

	if(packet->message->quality_of_service == 0)
	{
		if(pico_mqtt_list_push_back(mqtt->input_queue, packet) == ERROR)
			return ERROR;
	}

	if(packet->message->quality_of_service == 1)
	{
		if(!pico_mqtt_list_contains(mqtt->input_queue, (uint16_t) packet->packet_id))
		{
			if(pico_mqtt_list_push_back(mqtt->input_queue, packet) == ERROR)
				return ERROR;
		}
	}

	if(packet->message->quality_of_service == 2)
	{
		if(pico_mqtt_list_push_back(mqtt->wait_queue, packet) == ERROR)
			return ERROR;
		if(send_acknowledge(mqtt, packet, 0) == ERROR)
			return ERROR;

		destroy_packet(packet);
	}

	return SUCCES;
}

static int handle_pubrec( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet )
{
	struct pico_mqtt_packet* waiting_packet = NULL;

	CHECK_NOT_NULL(mqtt);
	CHECK(packet->type == PUBREC, "Wrong handler for message.\n");

	if(packet->packet_id >= 0)
		waiting_packet = pico_mqtt_list_get(mqtt->wait_queue, (uint16_t) packet->packet_id);

	if(send_acknowledge(mqtt, packet, is_trigger_message(mqtt, waiting_packet)) == ERROR)
		return ERROR;


	destroy_packet(waiting_packet);	
	destroy_packet(packet);
	return SUCCES;
}
			
static int handle_pubrel( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet )
{
	struct pico_mqtt_packet* waiting_packet = NULL;

	CHECK_NOT_NULL(mqtt);
	CHECK(packet->type == PUBREL, "Wrong handler for message.\n");

	if(packet->packet_id >= 0)
		waiting_packet = pico_mqtt_list_get(mqtt->wait_queue, (uint16_t) packet->packet_id);

	if(send_acknowledge(mqtt, packet, 0) == ERROR)
		return ERROR;

	if(pico_mqtt_list_push_back(mqtt->input_queue, waiting_packet) == ERROR)
		return ERROR;

	destroy_packet(waiting_packet);	
	destroy_packet(packet);
	return SUCCES;
}

static int handle_acknowledge( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet )
{
	struct pico_mqtt_packet* waiting_packet = NULL;

	CHECK_NOT_NULL(mqtt);
	CHECK((packet->type == PUBACK) ||
		(packet->type == PUBCOMP) ||
		(packet->type == SUBACK) ||
		(packet->type == UNSUBACK), "Wrong handler for message.\n");

	if(packet->packet_id >= 0)
		waiting_packet = pico_mqtt_list_get(mqtt->wait_queue, (uint16_t) packet->packet_id);

	if(waiting_packet != NULL)
	{
		check_if_request_complete(mqtt, waiting_packet);
		destroy_packet(waiting_packet);
	}

	destroy_packet(packet);
	return SUCCES;
}

static int handle_pingreq( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet )
{
	CHECK_NOT_NULL(mqtt);
	CHECK((packet->type == PINGREQ), "Wrong handler for message.\n");

	if(send_acknowledge(mqtt, packet, 0) == ERROR)
		return ERROR;

	destroy_packet(packet);
	return SUCCES;
}

static int handle_pingresp( struct pico_mqtt* mqtt, struct pico_mqtt_packet* packet )
{
	CHECK_NOT_NULL(mqtt);
	CHECK((packet->type == PINGRESP), "Wrong handler for message.\n");

	destroy_packet(packet);
	return SUCCES;
}