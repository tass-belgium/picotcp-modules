/*******************************************************************************
 * @file humiditySensor.c
 * @description MQTT client implementation:
 * 				PUBLISH:   TOPIC_HUM
 * 				SUBSCRIBE: TOPIC_SPR [optional]
 * @details     This humidity sensor will offer humidity values to the MQTT
 * 				network. Anyone subscribed to the TOPIC_HUM topic will receive
 * 				the values.
 *
 * 				This sensor will publish to a generic topic instead of including
 * 				a device specific ID. The ID will instead be offered in the
 * 				message itself. Other MQTT clients subscribed to this generic
 * 				topic require extra processing to identify the message, before
 * 				using or discarding the data.
 ******************************************************************************/
#include "pico_mqtt.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "time.h"


/*******************************************************************************
 * MACROS
 ******************************************************************************/
#define LOCALHOST	0				// Trick to compare in the preprocessor (no string comparison possible)
#define HOSTNAME 	LOCALHOST 		// Hostname used by MQTT server in the network: e.g. "10.222.23.157"
/* Offer warning when compiling with localhost...*/
#if HOSTNAME == LOCALHOST
	#undef HOSTNAME
	#define HOSTNAME 	"localhost"
	#warning HOSTNAME set to "localhost", are you sure you are running a local MQTT server?
#endif
#define PORT 		"1883"				// Port     used by MQTT server

#define USER 		"IOT"				// Credentials needed to connect with the MQTT broker
#define PWD  		"fundamentals"		// Credentials needed to connect with the MQTT broker

#define QOS			0					// Quality of service for the MQTT communication [0 default, 1, 2]
										// NOTE: QOS higher then 0 needs work
#define TOPIC_HUM 	"garden/humidity"	// Topic used to send humidity data
#define TOPIC_SPR 	"garden/sprinkler"	// Topic used to send humidity data


/*******************************************************************************
 * Type definitions
 ******************************************************************************/
typedef enum program_t {
	program_simple_humidity_sensor,
	program_interactive_humidity_sensor,
	program_invalid = 0xff
} program_t;


/*******************************************************************************
 * Function declarations
 ******************************************************************************/
/* Program functions*/
static void main_humidity_sensor( struct pico_mqtt* mqtt, char* sensor_name );
static void main_better_humidity_sensor( struct pico_mqtt* mqtt, char* sensor_name );
/* Helper functions*/
static void sleep_override( uint16_t seconds );
static void sig_handler( int signo );
static void help( char* prog_name );
static void print_humidity( uint8_t humidity, char* sensor_name );
static int get_value_via_key( char* source, char* key, uint32_t max_value_length, char* value );

/*******************************************************************************
 * Static data
 ******************************************************************************/
static int running = 1;        // running can be changed in the global scope to halt the execution


/*******************************************************************************
 * Function definitions
 ******************************************************************************/

/**
 * @brief Print program help
 */
static void help( char* prog_name )
{
	printf("Usage:\n");
	printf("======\n");
	printf("To publish random humidity data\n\t%s [yourName] random\n", prog_name);
	printf("To publish interactive humidity data\n\t%s [yourName] interactive\n", prog_name);
}

/**
 * @brief Signal handler
 *        Will break out of while loop to allow proper disconnection from the MQTT network
 */
static void sig_handler(int signo)
{
    if (signo == SIGINT)
    {
        printf("HALT received...\nShutting down\n");
        running = 0;
    }
}

/**
 * @brief Do not perform any action for \p seconds
 *        Check every 500 ms if the loop should be broken (via the 'running' variable)
 */
static void sleep_override( uint16_t seconds )
{
    uint32_t count = seconds * 2;

    // Check every half a second if we should still sleep
    for (; running && count > 0; --count)
    {
        usleep(500000);
    }
}

/**
 * @brief Print local sensor data (debug purposes)
 */
static void print_humidity( uint8_t humidity, char* sensor_name )
{
	printf("[LOCAL] sensor%s : %u %% humidity\n", sensor_name, humidity);
}

/**
 * @brief Parse a JSON-like \p source string.
 * 		  Find a \p key, return the matching value via \p value.
 * @note: only works for " delimited strings: e.g. {"key":"value"}
 * @return 0 on success
 *         -1 when key not found or value did not start with a "
 *         -2 when value did not end with a "
 */
static int get_value_via_key( char* source, char* key, uint32_t max_value_length, char* value )
{
	char lookup_key[128];
	memset(lookup_key, 0, 128);
	snprintf(lookup_key, 128, "\"%s\":\"", key);

	// Search for lookup_key in source, return pointer to start of lookup_key
	char* start_value = strstr(source, lookup_key);

	if (start_value == NULL)
	{
		// Key not found
		// or value did not start with "
		return -1;
	}

	// Set start of value to the end of the lookup_key
	start_value += strlen(lookup_key);

	// Search for delimiting " at the end of the value
	char* end_value = strchr(start_value,'"');

	if (end_value == NULL)
	{
		// Value did not end with a "
		return -2;
	}
	memset(value, 0, max_value_length);
	uint32_t length = (end_value - start_value) % max_value_length;
	strncpy(value,start_value, length);

	return 0;
}


/**
 * @brief MQTT client that publishes humidity data to the connected \p mqtt
 *        server on topic "garden/humidity" in JSON format:
 *        {"Name":"[sensor_name]","value":"[humidity_sensor_data_in_percentage]"}
 */
static void main_humidity_sensor( struct pico_mqtt* mqtt, char* sensor_name )
{
	struct pico_mqtt_message* message = NULL;
	uint8_t humidity = 0;
	char buffer[128];

	// Clear buffer
	memset(buffer, 0, sizeof(buffer));

	// Seed the random generator
	srand(time(NULL));


    running = 1;
	while(running)
	{
		// Send a random number between 0-99 ever 7 seconds
		humidity = (uint8_t)(rand() % 100);

		// Build JSON message
		snprintf(buffer, 128, "{\"Name\":\"%s\",\"Value\":\"%03d\"}\n", sensor_name, humidity);

		// [MQTT] Create mqtt message
		message = pico_mqtt_create_message(TOPIC_HUM, buffer, strlen(buffer));
		pico_mqtt_message_set_quality_of_service(message, QOS);

		// [MQTT] PUBLISH MESSAGE on topic
		pico_mqtt_publish(mqtt, message, 5000);

		// [MQTT] Clear message + internal buffers
		pico_mqtt_destroy_message(message);
		message = NULL;

		// Print sensor data locally
		print_humidity(humidity, sensor_name);
		sleep_override(7);
	}

	return;
}

/**
 * @brief This humidity sensor is not randomized,
 *        When your sprinkler is turned "OFF", the humidity will gradually decrease over time,
 *        When the sprinkler is turned "ON", the humidity will gradually increase over time
 */
static void main_better_humidity_sensor( struct pico_mqtt* mqtt, char* sensor_name )
{
	struct pico_mqtt_message* message = NULL;
	uint8_t humidity = 50;
	uint8_t temperature = 20;
	char buffer[128];
	uint32_t sprinkler_state = 0;

	// Clear buffer
	memset(buffer, 0, sizeof(buffer));

	// [MQTT] SUBSCRIBE to topic TOPIC_SPR
	if (pico_mqtt_subscribe(mqtt, TOPIC_SPR, QOS, 5000))
	{
		printf("Failed to subscribe on topic %s", TOPIC_HUM);
		return;
	}

    running = 1;
	while(running)
	{
		// Update humidity based on sprinkler state and previous value
		if (sprinkler_state)
		{
			// Sprinkler ON, humidity should be increasing
			humidity = (humidity + 6  * temperature / 20) % 100;
		}
		else
		{
			// Sprinkler OFF, humidity should be decreasing
			humidity = (humidity - 7 * temperature / 20) % 100 ;
		}

		// Build JSON message
		snprintf(buffer, 128, "{\"Name\":\"%s\",\"Value\":\"%03d\"}\n", sensor_name, humidity);

		// [MQTT] Create mqtt message
		message = pico_mqtt_create_message(TOPIC_HUM, buffer, strlen(buffer));
		pico_mqtt_message_set_quality_of_service(message, QOS);

		// [MQTT] PUBLISH MESSAGE on topic
		pico_mqtt_publish(mqtt, message, 5000);

		// [MQTT] Clear message + internal buffers
		pico_mqtt_destroy_message(message);
		message = NULL;

		// Print sensor data locally
		print_humidity(humidity, sensor_name);


		uint32_t seconds_to_wait = 7;

		/***********************************************************************
		 * POLL SPRINKLER STATE (via subscribed topic)
		 **********************************************************************/
	    // Check every half a second if we should still sleep
	    for (seconds_to_wait*=2; running && seconds_to_wait > 0; --seconds_to_wait)
	    {
	    	// [MQTT] READ message (if any)
	        pico_mqtt_receive(mqtt, &message, 500); // blocking wait for max 500 ms for a message
	        if(message != NULL)
			{
	        	// Validate the ID (Name) of the message matches our own actuator_name
				/* TODO IoTFundamentalist
				 *      Use the ID of your garden in the topic
				 *      This pushes the parsing of the ID to the MQTT internal working
				 *      instead of validating it in the user application
				 */
				int result = get_value_via_key(message->data->data, "Name", 128, buffer);

				if (result || strncmp(sensor_name, buffer, 128))
				{
					printf("Ignoring data from: %s\n", buffer);
				}
				else
				{
					get_value_via_key(message->data->data, "Value", 128, buffer);
					printf("Received data from your sprinkler >> %s\n", buffer);

					if (strncmp(buffer, "ON", 128) == 0)
					{
						sprinkler_state = 1;
					}
					else
					{
						sprinkler_state = 0;
					}
				}
			}
	        // [MQTT] Clear the message and internal buffering
			pico_mqtt_destroy_message(message);
			message = NULL;
	    }
	}

	// [MQTT] UNSUBSCRIBE from topic TOPIC_SPR
	pico_mqtt_unsubscribe(mqtt, TOPIC_SPR, 2000);

	return;
}


/**
 * @brief: the start of the MQTT client program
 * @usage: To publish "garden/humidity" data: ./PROG_NAME [yourName] random
 *         To consume "garden/humidity" data: ./PROG_NAME [yourName] interactive
 */
int main( int argc, char* argv[] )
{
    struct pico_mqtt* mqtt = NULL;
	program_t activeProgram = program_invalid;
    char* sensor_name = argv[1];
	char client_id[128];

	/***************************************************************************
	 * Parse program arguments
	 **************************************************************************/
	if (argc != 3)
	{
		help(argv[0]);
		return -1;
	}

	if (strcmp(argv[2],"random") == 0)
	{
		activeProgram = program_simple_humidity_sensor;
	}
	else if (strcmp(argv[2], "interactive") == 0)
	{
		activeProgram = program_interactive_humidity_sensor;
	}
	else
	{
		help(argv[0]);
		return -1;
	}

    // Install signal handler on ^C
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        printf("\ncan't catch SIGINT\n");
    }

	/***************************************************************************
	 * [MQTT] Connect this client to the MQTT server on the network
	 **************************************************************************/
    printf("Starting the MQTT client\n");

	mqtt = pico_mqtt_create();

	strncpy(client_id, "sensor", 128);
	strncat(client_id, sensor_name, 128);
	if(pico_mqtt_set_client_id( mqtt, client_id))
	{
		printf("An error occurred while setting the client_id\n");
	}

	if(pico_mqtt_set_username( mqtt, USER))
	{
		printf("An error occurred while setting the username\n");
	}

	if(pico_mqtt_set_password( mqtt, PWD))
	{
		printf("An error occured while setting the password\n");
	}

	if(pico_mqtt_connect(mqtt, HOSTNAME, PORT, 10))
	{
		printf("An error occured during connecting\n");
	}

	printf("clientId: %s successfully connected to server %s:%s\n", client_id, HOSTNAME, PORT);


	/***************************************************************************
	 * [MQTT] Start an MQTT program (while loop)
	 **************************************************************************/
	switch(activeProgram)
	{
	case program_simple_humidity_sensor:
		printf("Starting the simple humidity sensor\n");
		//This will start an endless loop
		main_humidity_sensor( mqtt, sensor_name );
		break;
	case program_interactive_humidity_sensor:
		printf("Starting the interactive humidity sensor\n");
		// This will read humidity sensor data, print it and publish sprinkler data
		main_better_humidity_sensor( mqtt, sensor_name );
		break;
	default:
		// An error occurred
		break;
	}


	/***************************************************************************
	 * [MQTT] Break connection and clear resources
	 **************************************************************************/
	pico_mqtt_disconnect( mqtt );
	pico_mqtt_destroy( mqtt );

	return 0;
}

