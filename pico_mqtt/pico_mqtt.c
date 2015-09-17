#include "pico_mqtt.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_error.h"

/**
* Data Structures
**/

/**
* Private Function Prototypes
**/

static int initialize( struct pico_mqtt** mqtt);
static int check_uri( const char* uri );

/**
* Public Function Implementation
**/

// allocate memory for the pico_mqtt object and freeing it
int pico_mqtt_create(struct pico_mqtt** mqtt, const char* uri, uint32_t* time_left)
{
	int result = 0;
	
	if(mqtt == NULL)
	{
		PERROR("Invallid input, MQTT object pointer not specified.\n");
		return ERROR;
	}

	if(time_left == NULL)
	{
		PERROR("Invallid input, time left is not specified.\n");
		return ERROR;
	}

	if(uri == NULL)
	{
		PTODO("Check that NULL is accepted as a URI.\n");
		PWARNING("No URI specified, connection will default to localhost.\n");
	}

	*mqtt = NULL;

	PTODO("Check the code styles and choose the prefered one.\n");	

	result = initialize( &mqtt );
	RETURN_IF_ERROR(result);

	RETURN_IF_ERROR(check_uri( uri ));

	result = pico_mqtt_stream_create( *mqtt, uri, time_left);
	if( result == ERROR )
	{
		return ERROR;
	}

	if( time_left == 0 )
	{
		PTODO("Add code to check if a timeout occured during object creation.\n");
		return ERROR;
	}

	return SUCCES;
}

int pico_mqtt_destory(struct pico_mqtt** mqtt);

// connect to the server and disconnect again
int pico_mqtt_connect(struct pico_mqtt* mqtt, uint32_t* timeout);
int pico_mqtt_disconnect(struct pico_mqtt* mqtt);

// ping the server and flush buffer and subscribtions (back to state after create)
int pico_mqtt_restart(struct pico_mqtt* mqtt, uint32_t* timeout);
int pico_mqtt_ping(struct pico_mqtt* mqtt, uint32_t* timeout );

// receive a message or publis a message
int pico_mqtt_receive(struct pico_mqtt* mqtt, uint32_t* timeout);
int pico_mqtt_publish(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, uint32_t* timeout);

//subscribe to a topic (or multiple topics) and unsubscribe
int pico_mqtt_subscribe(struct pico_mqtt* mqtt, const char* topic, const uint8_t quality_of_service, uint32_t* timeout); //return errors, qos is provided in message upon receive
int pico_mqtt_unsubscribe(struct pico_mqtt* mqtt, const char* topic, uint32_t* timeout);

/**
* Helper Function Implementation
**/

// free custom data types
int pico_mqtt_destroy_message(struct pico_mqtt_message * message);
int pico_mqtt_destroy_data(struct pico_mqtt_data * data);

// create and destroy custom data types
int pico_mqtt_create_data(struct pico_mqtt_data* result, const void* data, const uint32_t length);
int pico_mqtt_create_message(struct pico_mqtt_message* result, uint8_t header, uint16_t message_id, struct pico_mqtt_data topic, struct pico_mqtt_data data);

// get the last error according to the documentation (MQTT specification)
char* pico_mqtt_get_normative_error( const struct pico_mqtt* mqtt);
uint32_t pico_mqtt_get_error_documentation_line( const struct pico_mqtt* mqtt);

// get the protocol version
const char* pico_mqtt_get_protocol_version( void );

/**
* Private Functions Implementation
**/

static int initialize( struct pico_mqtt** mqtt)
{
	*mqtt = (struct pico_mqtt*) malloc(sizeof(struct pico_mqtt));
#ifdef DEBUG
	if(mqtt == NULL)
	{
		PERROR("Failed to allocate the memory for the pico_mqtt object\n");
		return ERROR;
	}
#endif
	PTODO("Initiliaze MQTT to default values");

	result = pico_mqtt_list_create( &((*mqtt)->output_queue) );
	if( result == ERROR )
	{
		return ERROR;
	}

	result = pico_mqtt_list_create( &((*mqtt)->active_messages) );
	if( result == ERROR )
	{
		return ERROR;
	}

	return SUCCES;
}

static int check_uri( const char* uri )
{
	PTODO("This functions is not yet implemented.\n");
	return SUCCES;
}

#endif
