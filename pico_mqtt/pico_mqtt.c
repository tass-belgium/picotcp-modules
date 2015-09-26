#include "pico_mqtt.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_error.h"

/**
* Data Structures
**/

/**
* Private Function Prototypes
**/

static int initialize( struct pico_mqtt** mqtt );
static int destroy( struct pico_mqtt** mqtt);
static int check_uri( const char* uri );
static struct timeval ms_to_timeval(uint32_t timeout);
/* if this message is finished the function will return */
/*static int set_trigger_message( struct pico_mqtt* mqtt, struct pico_mqtt_message* trigger_message);*/

/**
* Public Function Implementation
**/

/* allocate memory for the pico_mqtt object and free it */
struct pico_mqtt* pico_mqtt_create( void )
{
	struct pico_mqtt* mqtt = NULL;
	int result = 0;

	result = initialize( &mqtt );
	if( result == ERROR )
	{
		PTRACE;
		return NULL;
	}

	PINFO("succesfully created the MQTT object.\n");
	return mqtt;
}

void pico_mqtt_destroy(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PWARNING("The pointer is NULL, no MQTT object to destroy.\n");
		return;
	}

	destroy(&mqtt);

	return;
}

/* getters and setters for the connection options, resenable defaults will be used */
int pico_mqtt_set_client_id(struct pico_mqtt* mqtt, const char* client_id_string)
{
	struct pico_mqtt_data* client_id = NULL;

	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return ERROR;
	}

	client_id = pico_mqtt_string_to_data( client_id_string );
	if(client_id == NULL)
	{
		PTRACE;
		return ERROR;
	}

	mqtt->client_id = client_id;
	return SUCCES;
}

int pico_mqtt_is_client_id_set(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return -1;
	}

	return mqtt->client_id != NULL;
}

struct pico_mqtt_data* pico_mqtt_get_client_id(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return NULL;
	}

	return mqtt->client_id;
}

int pico_mqtt_set_will_message(struct pico_mqtt* mqtt, const struct pico_mqtt_message will_message);
int pico_mqtt_is_will_message_set(struct pico_mqtt* mqtt);
struct pico_mqtt_data* pico_mqtt_get_will_message(struct pico_mqtt* mqtt);

int pico_mqtt_set_username(struct pico_mqtt* mqtt, const char* username_string)
{
	struct pico_mqtt_data* username = NULL;

	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return ERROR;
	}

	username = pico_mqtt_string_to_data( username_string );
	if(username == NULL)
	{
		PTRACE;
		return ERROR;
	}

	mqtt->username = username;
	return SUCCES;
}

int pico_mqtt_is_username_set(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return -1;
	}

	return mqtt->username != NULL;
}

struct pico_mqtt_data* pico_mqtt_get_username(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return NULL;
	}

	return mqtt->username;
}

int pico_mqtt_set_password(struct pico_mqtt* mqtt, const char* password_string)
{
	struct pico_mqtt_data* password = NULL;

	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return ERROR;
	}

	password = pico_mqtt_string_to_data( password_string );
	if(password == NULL)
	{
		PTRACE;
		return ERROR;
	}

	mqtt->password = password;
	return SUCCES;
}

int pico_mqtt_is_password_set(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return -1;
	}

	return mqtt->password != NULL;
}

struct pico_mqtt_data* pico_mqtt_get_password(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return NULL;
	}

	return mqtt->password;
}

/* connect to the server and disconnect again */
int pico_mqtt_connect(struct pico_mqtt* mqtt, const char* uri, const char* port, const uint32_t timeout)
{
	int result = 0;
	struct pico_mqtt_message* message = NULL;
	struct timeval time_left = ms_to_timeval(timeout);
	uint8_t flag = 0;

	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return ERROR;
	}

	if(uri == NULL)
	{
		PTODO("Check that NULL is accepted as a URI.\n");
		PWARNING("No URI is specified (NULL), connection will default to localhost.\n");
	}

	PTODO("write code to use a default port or service, readup on services.\n");
	result = check_uri( uri );
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}

	result = pico_mqtt_stream_connect( mqtt, uri, port );
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}

#if ALLOW_EMPTY_CLIENT_ID == 0
	if( (mqtt->client_id.length == 0) || (mqtt->client_id.data == NULL) )
	{
		PERROR("Client id is not set, the configuration requires a client id ( ALLOW_EMPTY_CLIENT_ID = 0).\n");
		return ERROR;
	}
#endif /* ALLOW_EMPTY_CLIENT_ID == 0 */

	result = pico_mqtt_serializer_create_connect(
		mqtt,
		&message,
		mqtt->keep_alive_time,
		mqtt->retain,
		mqtt->quality_of_service,
		1, /* Clean session when connecting */
		mqtt->client_id,
		mqtt->will_topic,
		mqtt->will_message,
		mqtt->username,
		mqtt->password);
	
	if(result == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	result = pico_mqtt_stream_set_output_message( mqtt, message);
	if(result == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	result = pico_mqtt_stream_send_receive( mqtt, &time_left);
	if(result == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	PINFO("Send the connect message.\n");

	result = pico_mqtt_stream_is_input_message_set( mqtt, &flag);
	if(result == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(flag == 0)
	{
		PERROR("A timeout occured, could not connect to the server in the specified time.\n");
		return ERROR;
	}

	PTODO("Check the incomming message to see if it is the connack message");
	PINFO("Connected succesfully to the server.\n");

	return SUCCES;
}

int pico_mqtt_disconnect(struct pico_mqtt* mqtt);

/* ping the server and flush buffer and subscribtions (back to state after create) */
int pico_mqtt_restart(struct pico_mqtt* mqtt, const uint32_t timeout); /* optional function */
int pico_mqtt_ping(struct pico_mqtt* mqtt, const uint32_t timeout );

/* receive a message or publis a message *//*
int pico_mqtt_receive(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, const uint32_t timeout)
{
		result = 0;
	length = 0;

	CHECK_MQTT();
	CHECK_TIME_LEFT();

	if(message == NULL)
	{
		PERROR("Invallid input: No return pointer for the incomming message specified.\n");
		return ERROR;
	}

#ifdef DEBUG
	if(mqtt->completed_messages == NULL)
	{
		PERROR("Incomplete messages queue should be specified.\n");
		return ERROR;
	}
#endif

	result = pico_mqtt_list_length(mqtt->completed_messages, &length);
	RETURN_IF_ERROR(result);

	if(length > 0) *//*their are already completed messages*//*
	{
		RETRUN_IF_ERROR(pico_mqtt_list_pop(mqtt->completed_messages, message));
		return SUCCES;
	}

	return SUCCES;
}
*/
int pico_mqtt_publish(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, const uint32_t timeout);

/* subscribe to a topic (or multiple topics) and unsubscribe */
int pico_mqtt_subscribe(struct pico_mqtt* mqtt, const char* topic, const uint8_t quality_of_service, const uint32_t timeout);
int pico_mqtt_unsubscribe(struct pico_mqtt* mqtt, const char* topic, const uint32_t timeout);

/**
* Helper Function Prototypes
**/

/* free custom data types */
void pico_mqtt_destroy_message(struct pico_mqtt_message * message);
void pico_mqtt_destroy_data(struct pico_mqtt_data * data)
{
	if(data == NULL)
	{
		PWARNING("Attempting to free NULL.\n");
		return;
	}

	if(data->data == NULL)
	{
		free(data);
	} else
	{
		free(data->data);
		free(data);
	}

	return;
}

/* create and destroy custom data types */
struct pico_mqtt_data* pico_mqtt_create_data(const void* data, const uint32_t length);

struct pico_mqtt_data* pico_mqtt_string_to_data(const char* string)
{
	uint32_t i = 0;
	const char* index = string;
	struct pico_mqtt_data* data = NULL;

	if(string == NULL)
	{
		PERROR("Invallid input, the string is not set.\n");
		return NULL;
	}

	data = (struct pico_mqtt_data*) malloc(sizeof(struct pico_mqtt_data));
	if(data == NULL)
	{
		PERROR("Unable to allocate memory for the data object.\n");
		return NULL;
	}

	while(*index != '\0')
	{
		++(data->length);
		++index;

		if(data->length > MAXIMUM_STRING_LENGTH)
		{
			PERROR("String to long, please check MAXIMUM_STRING_LENGTH in configuration file.\n");
			free(data);
			return NULL;
		}
	}

	data->data = malloc(data->length);

	for(i=0; i<data->length; ++i)
	{
		*((char*) data->data + i) = *(string + i);
	}

	return data;
}
struct pico_mqtt_message pico_mqtt_create_message(const char* topic, struct pico_mqtt_data data);

/* getters and setters for message options */
int pico_mqtt_message_set_quality_of_service(struct pico_mqtt_message* message, int quality_of_service);
int pico_mqtt_message_get_quality_of_service(struct pico_mqtt_message* message);

int pico_mqtt_message_set_retain(struct pico_mqtt_message* message, int quality_of_service);
int pico_mqtt_message_get_retain(struct pico_mqtt_message* message);

int pico_mqtt_message_is_duplicate_flag_set(struct pico_mqtt_message* message);
char* pico_mqtt_message_get_topic(struct pico_mqtt_message* message);


// get the last error according to the documentation (MQTT specification)
char* pico_mqtt_get_normative_error( const struct pico_mqtt* mqtt);
uint32_t pico_mqtt_get_error_documentation_line( const struct pico_mqtt* mqtt);

// get the protocol version
const char* pico_mqtt_get_protocol_version( void )
{
	static const char version = '4';
	return &version;
}

/**
* Private Functions Implementation
**/
/*
static int set_trigger_message( struct pico_mqtt* mqtt, struct pico_mqtt_message* trigger_message)
{
#ifdef DEBUG
	if((mqtt == NULL) || (trigger_message == NULL))
	{
		PERROR("Invallid input.\n");
		return ERROR;
	}
#endif

	mqtt->trigger_message == message;
	return SUCCES;
}*/

static int initialize( struct pico_mqtt** mqtt )
{
	int result = 0;
	*mqtt = (struct pico_mqtt*) malloc(sizeof(struct pico_mqtt));

#ifdef DEBUG
	if(mqtt == NULL)
	{
		PERROR("Failed to allocate the memory for the pico_mqtt object\n");
		return ERROR;
	}
#endif
	PTODO("Initiliaze MQTT to default values");

	**mqtt = (struct pico_mqtt)
	{
		// stream
		.stream = NULL,

		// serializer
		.serializer = NULL,
		.next_message_id = 0,

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1
		// message buffers
		.trigger_message = NULL,
		.output_queue = NULL,
		.active_messages = NULL,
		.completed_messages = NULL,
#endif /* ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1 */

		// connection related
		.keep_alive_time = 0,
		.retain = 0,
		.quality_of_service = 0,
		.client_id = NULL,
		.will_topic= NULL,
		.will_message = NULL,
		.username = NULL,
		.password = NULL,

		// memory
		.malloc = NULL,
		.free = NULL,

		// status
		.connected = 0,
		.normative_error = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		.documentation_reference = 0,
	};

	PTODO("combine the will message and topic to a message struct.\n");

	result = pico_mqtt_stream_create( *mqtt);
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1

	result = pico_mqtt_list_create( &((*mqtt)->output_queue) );
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}

	result = pico_mqtt_list_create( &((*mqtt)->active_messages) );
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}
	
	result = pico_mqtt_list_create( &((*mqtt)->completed_messages) );
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}

#endif /* ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1 */

	return SUCCES;
}

static int destroy( struct pico_mqtt** mqtt)
{
	int result = 0;

#ifdef DEBUG
	if(mqtt == NULL)
	{
		PERROR("The MQTT object is NULL.\n");
		return ERROR;
	}
#endif

	result = pico_mqtt_stream_destroy(*mqtt);
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1

	result = pico_mqtt_list_destroy( &((*mqtt)->output_queue) );
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}

	result = pico_mqtt_list_destroy( &((*mqtt)->active_messages) );
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}
	
	result = pico_mqtt_list_destroy( &((*mqtt)->completed_messages) );
	if( result == ERROR )
	{
		PTRACE;
		return ERROR;
	}

#endif /* ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1 */

	PTODO("free connection information if not null, check if you want to keep the warnings\n");

	if((*mqtt)->client_id != NULL)
	{
		pico_mqtt_destroy_data((*mqtt)->client_id);
	}

	if((*mqtt)->will_topic != NULL)
	{
		pico_mqtt_destroy_data((*mqtt)->will_topic);
	}

	if((*mqtt)->will_message != NULL)
	{
		pico_mqtt_destroy_data((*mqtt)->will_message);
	}

	if((*mqtt)->username != NULL)
	{
		pico_mqtt_destroy_data((*mqtt)->username);
	}

	if((*mqtt)->password != NULL)
	{
		pico_mqtt_destroy_data((*mqtt)->password);
	}

	free(*mqtt);
	*mqtt = NULL;

	return SUCCES;
}

static int check_uri( const char* uri )
{
	PTODO("This functions is not yet implemented.\n");
	uri++;
	return SUCCES;
}

static struct timeval ms_to_timeval(uint32_t timeout)
{
	return (struct timeval){.tv_sec = timeout/1000, .tv_usec = (timeout%1000) *1000};
}

#ifdef DEBUG
/**
* Debug Functions
**/

void pico_mqtt_print_data(const struct pico_mqtt_data* data)
{
	uint32_t column = 0;
	uint32_t row = 0;
	uint32_t index = 0;

	if( data ==  NULL )
	{
		printf("+---------------------------------------------------------+\n");
		printf("|  data: %12p                                     |\n", data);
		printf("+---------------------------------------------------------+\n");
		return;
	}

	printf("+---------------------------------------------------------+\n");
	printf("|  data: %14p                                   |\n", data);
	printf("|  length: %14d                                 |\n", data->length);
	printf("|  data pointer: %14p                           |\n", data->data);
	printf("+---------------------------------------------------------+\n");
	printf("|                                                         |\n");

	if( data->data == NULL)
	{
		printf("|                                                         |\n");
		printf("|                    No data to display                   |\n");
		printf("|                                                         |\n");
	} else
	{
		for(row=0; row <= data->length/10; row++)
		{
			printf("| %03d: ", index);	
			for(column=0; column<10; column++)
			{
				if( index >= data->length)
				{
					printf("     ");
				} else
				{
					printf("0x%02X ", *((uint8_t*) data->data + index));
				}

				++index;
			}
			printf(" |\n");
		}
	printf("|                                                         |\n");
	printf("+---------------------------------------------------------+\n");
	printf("|                                                         |\n");

		index = 0;
		for(row=0; row <= data->length/50; row++)
		{
			printf("| %03d: ", index);	
			for(column=0; column<50; column++)
			{
				if( index >= data->length)
				{
					printf(" ");
				} else
				{
					printf("%c", *((char*) data->data + index));
				}

				++index;
			}
			printf(" |\n");
		}
	}

	printf("|                                                         |\n");
	printf("+---------------------------------------------------------+\n");
	return;
}

#endif /* defined DEBUG */