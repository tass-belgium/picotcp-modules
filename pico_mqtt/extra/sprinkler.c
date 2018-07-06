/******************************************************************************
 * @file sprinkler.c
 * @description MQTT client implementation:
 * 				SUBSCRIBE: TOPIC_HUM
 * 				PUBLISH:   TOPIC_SPR
 * @details     This sprinkler will start watering your garden whenever your
 * 				humidity sensor offers a humidity of MIN_HUMIDITY or below.
 *
 * 				The definition of "your" equals the name of the user.
 * 				Both the humidity sensor and the sprinkler should have the same
 * 				username. Note this name should be unique in the MQTT network.
 *
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

#define MIN_HUMIDITY 40
#define MAX_HUMIDITY 70


/*******************************************************************************
 * Type definitions
 ******************************************************************************/
typedef enum program_t {
	program_sprinkler_actuator,
	program_invalid = 0xff
} program_t;


/*******************************************************************************
 * Function declarations
 ******************************************************************************/
/* Program functions*/
static int main_sprinkler( struct pico_mqtt* mqtt, char* actuator_name );
/* Helper functions*/
static void sig_handler( int signo );
static void help( char* prog_name );
static int get_value_via_key( char* source, char* key, uint32_t max_value_length, char* value );
static uint32_t is_watering_needed(char* state, uint8_t humidity );
static uint32_t calculate_needed_water( uint8_t humidity, uint8_t temperature, uint32_t volume_of_air_m3 );
static void water_garden( uint32_t liters );


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
static void help(char* prog_name)
{
	printf("Usage:\n");
	printf("======\n");
	printf("%s [yourName]\n", prog_name);
}

/**
 * @brief Signal handler
 *        Will break out of while loop to allow proper disconnection from the MQTT network
 */
static void sig_handler(int signo)
{
    if (signo == SIGINT)
    {
        printf("HALT received...\nShutting down (2s)\n");
        running = 0;
    }
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
 * @brief Checks if sprinkler should start or stop watering the garden
 */
static uint32_t is_watering_needed(char* state, uint8_t humidity )
{
	if (strcmp(state, "OFF") == 0)
	{
		// Sprinkler "OFF" and below MIN humidity?
		if (humidity <= MIN_HUMIDITY)
		{
			printf("[LOCAL] Your garden is getting thirsty... humidity: %d\n",humidity);
			return 1;
		}
	}
	else
	{
		// Sprinkler "ON" but not yet at MAX humidity?
		if (humidity < MAX_HUMIDITY)
		{
			printf("[LOCAL] Your garden is still thirsty... humidity: %d\n",humidity);
			return 1;
		}
	}

	return 0;
}

/**
 * @brief Calculate how much water needed to hydrate a \p volume_of_air_m3 to reach a desired humidity
 * @note  THIS IS NOT A VALID CALCULATION
 */
static uint32_t calculate_needed_water( uint8_t humidity, uint8_t temperature, uint32_t volume_of_air_m3)
{
	uint8_t required_humidity;
	float water_to_reach_required_humidity; // Amount of H2O (gram) per kg air to reach the desired relative humidity [g/kg]
	float adopted_weight_air; // Adopted weight of one cubic metre of air of [kg/m3]
	uint32_t liters_needed;

	required_humidity = MAX_HUMIDITY;

	water_to_reach_required_humidity = 6.0 * (required_humidity - humidity) / (humidity + 1);
	adopted_weight_air = 1.2 / (1 + (temperature - 20)/200);
	liters_needed = (float)(water_to_reach_required_humidity * adopted_weight_air * volume_of_air_m3);

	return liters_needed;
}

/**
 * @brief Output function to start the sprinkler
 *        Currently prints a string instead of squirting water from your PC.
 */
static void water_garden( uint32_t liters )
{
	printf("[LOCAL] Watering the garden now with %d liters\n", liters);
}


/**
 * @brief MQTT client that processes humidity data from the connected \p mqtt
 *        server on topic "garden/humidity".
 *        After processing, the sprinkler will inform the network by publishing its state in JSON format:
 *        {"Name":"[actuator_name]","value":"[state]"}
 */
static int main_sprinkler( struct pico_mqtt* mqtt, char* actuator_name )
{
	struct pico_mqtt_message* message = NULL;
	uint8_t humidity = 0;
	char buffer[128];
	char* state = "OFF";


	// [MQTT] SUBSCRIBE to topic TOPIC_HUM
	if (pico_mqtt_subscribe(mqtt, TOPIC_HUM, QOS, 5000))
	{
		printf("Failed to subscribe on topic %s", TOPIC_HUM);
		return -5;
	}

	running = 1;
	while (running)
	{
		/***********************************************************************
		 * POLL SPRINKLER STATE (via subscribed topic)
		 **********************************************************************/
		// [MQTT] READ message (if any)
		pico_mqtt_receive(mqtt, &message, 1000); // blocking wait for max 1 second for a message

		if(message != NULL)
		{
			// Validate the ID (Name) of the message matches our own actuator_name
			/* TODO IoTFundamentalist
			 *      Use the ID of your garden in the topic
			 *      This pushes the parsing of the ID to the MQTT internal working
			 *      instead of validating it in the user application
			 */
			int result = get_value_via_key(message->data->data, "Name", 128, buffer);

			if (result || strncmp(actuator_name, buffer, 128))
			{
				if (result)
				{
					printf("Received unreadable data: %s\n", (char*)message->data->data);
				}
				else
				{
					printf("Ignoring data from: %s\n", buffer);
				}

				// [MQTT] Clear the RECEIVED message and internal buffering
				pico_mqtt_destroy_message(message);
				message = NULL;
			}
			else
			{
				if (get_value_via_key(message->data->data, "Value", 128, buffer) != 0)
				{
					printf("Received unreadable data: %s\n", (char*)message->data->data);
					// [MQTT] Clear the RECEIVED message and internal buffering
					pico_mqtt_destroy_message(message);
					message = NULL;
					continue;
				}
				printf("Received humidity from your sensor >> %s\n", buffer);

				humidity = atoi(buffer);

				if (is_watering_needed(state, humidity))
				{
					water_garden(calculate_needed_water(humidity, 20, 4*10*2.5));
					// Update the sprinkler state to ON (will be published later on)
					state = "ON";
				}
				else
				{
					state = "OFF";
				}

				// [MQTT] Clear the RECEIVED message and internal buffering
				pico_mqtt_destroy_message(message);
				message = NULL;


				/***************************************************************
				 * PUBLISH SPRINKLER STATE
				 **************************************************************/
				// Build JSON message
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, 128, "{\"Name\":\"%s\",\"Value\":\"%s\"}\n", actuator_name, state);

				// [MQTT] Create mqtt message
				message = pico_mqtt_create_message(TOPIC_SPR, buffer, strlen(buffer));

				// [MQTT] PUBLISH MESSAGE on topic
				pico_mqtt_publish(mqtt, message, 1000);

				// [MQTT] Clear the PUBLISHED message and internal buffering
				pico_mqtt_destroy_message(message);
				message = NULL;
			}

		}
	}

	// [MQTT] UNSUBSCRIBE from topic TOPIC_HUM
	pico_mqtt_unsubscribe(mqtt, TOPIC_HUM, 2000);
	return 0;
}


/* @brief: the start of the MQTT client program
 * @usage: ./PROG_NAME [yourName]
 */
int main(int argc, char* argv[])
{
    struct pico_mqtt* mqtt = NULL;
	program_t activeProgram = program_invalid;
    char* actuator_name = argv[1];
    char client_id[128];

    /***************************************************************************
	 * Parse program arguments
	 **************************************************************************/
	if (argc != 2)
	{
		help(argv[0]);
		return -1;
	}
	else
	{
		activeProgram = program_sprinkler_actuator;
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

	strncpy(client_id, "actuator", 128);
	strncat(client_id, actuator_name, 128);
	if(pico_mqtt_set_client_id( mqtt, client_id))
	{
		printf("An error occured while setting the client_id\n");
	}

	if(pico_mqtt_set_username( mqtt, USER))
	{
		printf("An error occured while setting the username\n");
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
	int result = 0;
	switch(activeProgram)
	{
	case program_sprinkler_actuator:
		printf("=====================================\n");
		printf("Listening for data from sensor%s\n",actuator_name);
		//This will start an endless loop
		result = main_sprinkler( mqtt, actuator_name );
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

	return result;
}

