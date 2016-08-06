#include "pico_mqtt.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_debug.h"

/**
* Data Structures
**/

/**
* Private Function Prototypes
**/

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
	struct pico_mqtt* mqtt = (struct pico_mqtt*) MALLOC (sizeof(struct pico_mqtt));
	CHECK_NOT_NULL(mqtt);

	mqtt = PICO_MQTT_EMPTY;

	mqtt->stream = pico_mqtt_stream_create( &(mqtt->error) );
	CHECK_NOT_NULL(mqtt->stream);
	if( mqtt->stream == NULL )
	{
		pico_mqtt_destroy( mqtt );
		return NULL;
	}

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1

	mqtt->output_queue = pico_mqtt_list_create( &mqtt->error );
	CHECK_NOT_NULL(mqtt->output_queue);
	if( mqtt->output_queue == NULL )
	{
		pico_mqtt_destroy( mqtt );
		return NULL;
	}

	mqtt->wait_queue = pico_mqtt_list_create( &mqtt->error );
	CHECK_NOT_NULL(mqtt->wait_queue);
	if( mqtt->wait_queue == NULL )
	{
		pico_mqtt_destroy( mqtt );
		return NULL;
	}

	mqtt->input_queue = pico_mqtt_list_create( &mqtt->error );
	CHECK_NOT_NULL(mqtt->input_queue);
	if( mqtt->input_queue == NULL )
	{
		pico_mqtt_destroy( mqtt );
		return NULL;
	}

#endif /* ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1 */

	return mqtt;
}

void pico_mqtt_destroy(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
		return;

	pico_mqtt_stream_destroy(mqtt->stream);

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1

	pico_mqtt_list_destroy( mqtt->output_queue );
	pico_mqtt_list_destroy( mqtt->wait_queue );
	pico_mqtt_list_destroy( mqtt->input_queue );

#endif /* ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1 */

	pico_mqtt_destroy_data(mqtt->client_id);
	pico_mqtt_destroy_data(mqtt->will_topic);
	pico_mqtt_destroy_data(mqtt->will_message);
	pico_mqtt_destroy_data(mqtt->username);
	pico_mqtt_destroy_data(mqtt->password);

	FREE(mqtt);
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

	mqtt->error = NO_ERROR;

	PTODO("Validate string.\n");

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
		return ERROR;
	}

	mqtt->error = NO_ERROR;

	return mqtt->client_id != NULL;
}

struct pico_mqtt_data* pico_mqtt_get_client_id(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return NULL;
	}

	mqtt->error = NO_ERROR;

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

	mqtt->error = NO_ERROR;

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

	mqtt->error = NO_ERROR;

	return mqtt->username != NULL;
}

struct pico_mqtt_data* pico_mqtt_get_username(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return NULL;
	}

	mqtt->error = NO_ERROR;

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

	mqtt->error = NO_ERROR;

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

	mqtt->error = NO_ERROR;

	return mqtt->password != NULL;
}

struct pico_mqtt_data* pico_mqtt_get_password(struct pico_mqtt* mqtt)
{
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return NULL;
	}

	mqtt->error = NO_ERROR;

	return mqtt->password;
}

/* connect to the server and disconnect again */
int pico_mqtt_connect(struct pico_mqtt* mqtt, const char* uri, const char* port, uint32_t time_left)
{
	struct pico_mqtt_message* message = NULL;
	struct pico_mqtt_data connack = PICO_MQTT_DATA_ZERO;
	uint8_t session_present_flag = 0;
	uint8_t return_code = 0;
	uint8_t flag = 0;

	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return ERROR;
	}

	mqtt->error = NO_ERROR;

	if(uri == NULL)
	{
		PTODO("Check that NULL is accepted as a URI.\n");
		PWARNING("No URI is specified (NULL), connection will default to localhost.\n");
	}

	PTODO("write code to use a default port or service, readup on services.\n");
	if( check_uri(uri) == ERROR )
	{
		PTRACE;
		return ERROR;
	}

	if( pico_mqtt_stream_connect(mqtt, uri, port) == ERROR )
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

	if(pico_mqtt_serializer_create_connect(
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
		mqtt->password) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(pico_mqtt_stream_set_output_message( mqtt, message) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(pico_mqtt_stream_send_receive( mqtt, &time_left) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	PINFO("Send the connect message.\n");

	if(pico_mqtt_stream_is_input_message_set( mqtt->stream, &flag) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	PTODO("Allow the connack message to be confirmed later by a define.\n");
	if(flag == 0)
	{
		PERROR("A timeout occured, could not connect to the server in the specified time. Please try again.\n");
		return ERROR;
	}

	if(pico_mqtt_stream_get_input_message( mqtt->stream, &connack) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(pico_mqtt_deserialize_connack( mqtt, &connack, &session_present_flag, &return_code) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(session_present_flag)
	{
		mqtt->normative_error = "MQTT-3.2.2-1";
		PERROR("Normative error [MQTT-3.2.2-1]: A clean session was requested but the session flag was set.\n");
		return ERROR;
	}

	if(return_code != 0)
	{
#ifdef DEBUG
		if(return_code == 1)
		{
			PERROR("Connection Refused, unacceptable protocol version.\n");
		}

		if(return_code == 2)
		{
			PERROR("Connection Refused, identifier rejected.\n");
		}

		if(return_code == 3)
		{
			PERROR("Connection Refused, server unabailable.\n");
		}

		if(return_code == 4)
		{
			PERROR("Connection Refused, bad user name or password.\n");
		}

		if(return_code == 5)
		{
			PERROR("Connection Refused, not authorized.\n");
		}

		if(return_code > 5)
		{
			PERROR("Connection Refused, unknown return code. Check protocol versions or server implementation.\n");
		}
#endif
		return ERROR;
	}

	PINFO("Connected succesfully to the server.\n");

	return SUCCES;
}

int pico_mqtt_disconnect(struct pico_mqtt* mqtt);

/* ping the server and flush buffer and subscribtions (back to state after create) */
int pico_mqtt_restart(struct pico_mqtt* mqtt, const uint32_t timeout); /* optional function */
int pico_mqtt_ping(struct pico_mqtt* mqtt, const uint32_t timeout );

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
		return;

	if(data->data != NULL)
		FREE(data->data);

	FREE(data);
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
			FREE(data);
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

static int check_uri( const char* uri )
{
	PTODO("This functions is not yet implemented.\n");
	uri++;
	return SUCCES;
}

/**
* Main processing loop functions
**/

static int send_message(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, int time_left)
{
	mqtt->trigger_message = message;
	mqtt->trigger_on_receive = 0;

	if(pico_mqtt_list_push(mqtt->output_queue, message) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(main_loop() == ERROR)
	{
		PTRACE;
		return ERROR;
	}
}

static int receive_message(struct pico_mqtt* mqtt, struct pico_mqtt_message** message, int time_left)
{
	uint32_t length = 0;

	if(pico_mqtt_list_length(mqtt->input_queue, &length) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(length > 0)
	{
		if(pico_mqtt_list_pop(mqtt->input_queue, message) == ERROR)
		{
			PTRACE;
			return ERROR;
		} else {
			return SUCCES;
		}
	}

	mqtt->trigger_message = NULL;
	mqtt->trigger_on_receive = 1;

	if(main_loop(mqtt, time_left) == ERROR)
	{
		PTRACE;
		return ERROR;
	}
}

static int main_loop(struct pico_mqtt* mqtt, int time_left)
{
	struct pico_mqtt_message* message = NULL;

	while(time_left != 0)
	{
		if(mqtt->connected == 1){
			uint8_t output_message_set_flag = 0;
			uint8_t input_message_set_flag = 0;
			uint8_t message_ready_flag = 0;

			if(pico_mqtt_stream_is_output_message_set( mqtt->stream, &output_message_set_flag) == ERROR)
			{
				PTRACE;
				return ERROR;
			}

			if(!output_message_set_flag)
			{
				uint32_t length = 0;

				if(mqtt->active_output_message != NULL)
				{
					++mqtt->active_output_message->status;

					if(is_message_ready(mqtt, mqtt->active_output_message, &message_ready_flag) == ERROR)
					{
					PTRACE;
					return ERROR;
					}

					if(message_ready_flag)
					{
						if(remove_message(mqtt, mqtt->active_output_message) == ERROR)
						{
							PTRACE;
							return ERROR;
						}

						if(mqtt->active_output_message == mqtt->trigger_message)
						{
							mqtt->active_output_message = NULL;
							return SUCCES;
						}

						mqtt->active_output_message = NULL;

					}  else {
						if(process_message(mqtt, mqtt->active_output_message) == ERROR)
						{
							close_connection(mqtt);
							continue;
						}
					}
				}

				if(set_new_output_message(mqtt) == ERROR)
				{
					PTRACE;
					return ERROR;
				}
			}

			if(pico_mqtt_stream_is_input_message_set( mqtt->stream, &input_message_set_flag) == ERROR)
			{
				PTRACE;
				return ERROR;
			}

			if(input_message_set_flag == 1)
			{
				if(pico_mqtt_stream_get_input_message(mqtt->stream, &message) == ERROR)
				{
					PTRACE;
					return ERROR;
				}

				if(is_message_ready(mqtt, message, &message_ready_flag) == ERROR)
				{
					PTRACE;
					return ERROR;
				}

				if(message_ready_flag)
				{
					if(pico_mqtt_list_push(mqtt->input_queue, message) == ERROR)
					{
						PTRACE;
						return ERROR;
					}

					if(mqtt->trigger_on_receive)
					{
						return SUCCES;
					}
				} else {
					if(process_message(mqtt, message) == ERROR)
					{
						close_connection(mqtt);
						continue;
					}
				}
			}

			if(pico_mqtt_stream_send_receive(mqtt->stream, &time_left) == ERROR)
			{
				if(mqtt->error == CONNECTION_LOST)
				{
					mqtt->connected = 0;
					reconnect(mqtt);
				}

				if(mqtt->error == CONNECTION_BROKEN)
				{
					mqtt->connected = 0;
					return ERROR;
				}
			}
		} else {
			PTODO("write function to reconnect");
		}
	}
}

int set_new_output_message(struct pico_mqtt* mqtt)
{
	uint32_t length = 0;

	if(pico_mqtt_list_length(mqtt->output_queue, &length) == ERROR)
	{
		PTRACE;
		return ERROR;
	}

	if(length > 0)
	{
		if(pico_mqtt_list_pop(mqtt->output_queue, &mqtt->active_output_message) == ERROR)
		{
			PTRACE;
			return ERROR;
		}

		if(pico_mqtt_stream_set_output_message(mqtt->stream, message->data) == ERROR)
		{
			PTRACE;
			return ERROR;
		}
	}

	return SUCCES;
}

/**
* Debug Functions
**/

#ifdef DEBUG

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
