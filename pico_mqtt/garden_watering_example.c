#include "pico_mqtt.h"
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include <unistd.h>
#include <signal.h>

/*
                     _                            _            _
                    | |                          | |          (_)
  __ _  __ _ _ __ __| | ___ _ __   __      ____ _| |_ ___ _ __ _ _ __   __ _
 / _` |/ _` | '__/ _` |/ _ \ '_ \  \ \ /\ / / _` | __/ _ \ '__| | '_ \ / _` |
| (_| | (_| | | | (_| |  __/ | | |  \ V  V / (_| | ||  __/ |  | | | | | (_| |
 \__, |\__,_|_|  \__,_|\___|_| |_|   \_/\_/ \__,_|\__\___|_|  |_|_| |_|\__, |
  __/ |                                                                 __/ |
 |___/                                                                 |___/

 */

// CONFIGURATIONS
#define QOS			0					// Quality of service for the MQTT communication [0 default, 1, 2]
										// NOTE: QOS higher then 0 needs work
#define TOPIC_HUM 	"garden/humidity"	// Topic used to send humidity data
#define TOPIC_SPR 	"garden/sprinkler"	// Topic used to send humidity data

#define MIN_HUMIDITY 40
#define MAX_HUMIDITY 70

// Private functions
/* Helper*/
static int get_value_via_key( const char* source, const char* key, uint32_t max_value_length, char* value );
static void sig_handler( int signo );
/* Sprinkler*/
static uint32_t is_watering_needed( const char* state, uint8_t humidity );
static uint32_t calculate_needed_water( uint8_t humidity, uint8_t temperature, uint32_t volume_of_air_m3);
static void water_garden( uint32_t liters );
/* Humidity sensor*/
static void print_humidity( uint8_t humidity, const char* client_name);

// functions
int main_humidity_sensor( struct pico_mqtt* mqtt, const char* client_name);
int main_better_humidity_sensor( struct pico_mqtt* mqtt, const char* client_name );
int main_sprinkler( struct pico_mqtt* mqtt, const char* client_name );

static int running = 1;


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
static int get_value_via_key( const char* source, const char* key, uint32_t max_value_length, char* value )
{
	char lookup_key[128];
	char* start_value;
	char* end_value;
	uint32_t length;
	memset(lookup_key, 0, 128);
	snprintf(lookup_key, 128, "\"%s\":\"", key);

	// Search for lookup_key in source, return pointer to start of lookup_key
	start_value = strstr(source, lookup_key);

	if (start_value == NULL)
	{
		// Key not found
		// or value did not start with "
		return -1;
	}

	// Set start of value to the end of the lookup_key
	start_value += strlen(lookup_key);

	// Search for delimiting " at the end of the value
	end_value = strchr(start_value,'"');

	if (end_value == NULL)
	{
		// Value did not end with a "
		return -2;
	}
	memset(value, 0, max_value_length);
	length = (uint32_t)((end_value - start_value) % max_value_length);
	strncpy(value,start_value, length);

	return 0;
}

/**
 * @brief Checks if sprinkler should start or stop watering the garden
 */
static uint32_t is_watering_needed( const char* state, uint8_t humidity )
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

	water_to_reach_required_humidity = 6.0f * (float)(required_humidity - humidity) / (float)(humidity + 1);
	adopted_weight_air = 1.2f / (float)(1 + (temperature - 20)/200);
	liters_needed = (uint32_t)(water_to_reach_required_humidity * adopted_weight_air * (float)volume_of_air_m3);

	return liters_needed;
}

/**
 * @brief Output function to start the sprinkler
 *        Currently prints a string instead of squirting water from your PC.
 */
static void water_garden( uint32_t liters )
{
	printf("Watering the garden now with %d liters\n", liters);
}

/**
 * @brief Print local sensor data (debug purposes)
 */
static void print_humidity( uint8_t humidity, const char* client_name )
{
	printf("[LOCAL] Sensor_%s : %u %% humidity\n", client_name, humidity);
}

/**
 * @brief MQTT client that publishes humidity data to the connected @a mqtt
 *        server on topic "garden/humidity" in JSON format:
 *        {"Name":"[client_name]","Value":"[humidity_sensor_data_in_percentage]"}
 */
int main_humidity_sensor( struct pico_mqtt* mqtt, const char* client_name)
{
	struct pico_mqtt_message* message = NULL;
	uint8_t humidity = 0;
	char buffer[128];

	// Install signal handler on ^C
	signal(SIGINT, sig_handler);

	// Clear buffer
	memset(buffer, 0, sizeof(buffer));

	// Seed the random generator
	srand((unsigned int)time(NULL));

	running = 1;
	while(running)
	{
		// Send a random number between 0-99 ever 7 seconds
		humidity = (uint8_t)(rand() % 100);

		// Build JSON message
		snprintf(buffer, 128, "{\"Name\":\"%s\",\"Value\":\"%03d\"}\n", client_name, humidity);

		// Create mqtt message
		message = pico_mqtt_create_message(TOPIC_HUM, buffer, (uint32_t)strlen(buffer));
		pico_mqtt_message_set_quality_of_service(message, QOS);

		// Publish on topic
		pico_mqtt_publish(mqtt, message, 5000);

		// Clear message + internal buffers
		pico_mqtt_destroy_message(message);
		message = NULL;

		// Print sensor data locally
		print_humidity(humidity, client_name);
		sleep(7);
	}

	return 0;
}

/**
 * More efficient implementation:
 *
 * No memory allocations are made for each message (internally the data is copied
 * for sending). The Quality of Service has only to be set once.
 */
int main_better_humidity_sensor( struct pico_mqtt* mqtt, const char* client_name )
{
	struct pico_mqtt_message message = PICO_MQTT_MESSAGE_EMPTY;
	struct pico_mqtt_data data = PICO_MQTT_DATA_EMPTY;
	struct pico_mqtt_data topic = PICO_MQTT_DATA_EMPTY;
	char topic_string[] = TOPIC_HUM;
	uint8_t humidity = 0;
	char buffer[128];

	// Install signal handler on ^C
	signal(SIGINT, sig_handler);

	// Clear buffer
	memset(buffer, 0, sizeof(buffer));

	pico_mqtt_message_set_quality_of_service(&message, QOS);

	data.data = buffer;
	data.length = sizeof(buffer);	// This will always send the full buffer now ...
	message.data = &data;

	topic.data = topic_string;
	topic.length = sizeof(TOPIC_HUM) - 1; //don't include NULL termination
	message.topic = &topic;

	running = 1;
	while(running)
	{
		humidity = (uint8_t)(rand() % 100);
		// Build JSON message
		snprintf(buffer, 128, "{\"Name\":\"%s\",\"Value\":\"%03u\"}\n", client_name, humidity);
		pico_mqtt_publish(mqtt, &message, 5000);
		sleep(7);
	}

	return 0;
}

/**
 * @brief MQTT client that processes humidity data from the connected \p mqtt
 *        server on topic "garden/humidity".
 *        After processing, the sprinkler will inform the network by publishing its state in JSON format:
 *        {"Name":"[actuator_name]","Value":"[state]"}
 */
int main_sprinkler( struct pico_mqtt* mqtt, const char* client_name )
{
	struct pico_mqtt_message* message = NULL;
	uint8_t humidity = 49;
	char buffer[128];
	const char* state;

	state = "OFF";

	if (pico_mqtt_subscribe(mqtt, TOPIC_HUM, 1, 5000))
	{
		printf("Failed to subscribe on topic %s", TOPIC_HUM);
		return -5;
	}

	pico_mqtt_receive(mqtt, &message, 60000); // blocking wait for max 60 seconds for a message

	if(message != NULL)
	{
		if (get_value_via_key(message->data->data, "Value", 128, buffer) != 0)
		{
			printf("Received unreadable data: %s\n", (char*)message->data->data);
			// [MQTT] Clear the RECEIVED message and internal buffering
			pico_mqtt_destroy_message(message);
			message = NULL;
			pico_mqtt_unsubscribe(mqtt, TOPIC_HUM, 5000 );
			return 0;
		}
		printf("Received humidity from your sensor >> %s\n", buffer);

		humidity = (uint8_t)atoi(buffer);
		if (is_watering_needed(state, humidity))
		{
			water_garden(calculate_needed_water(humidity, 20, 4*10*2.5));
			// Update the sprinkler state to ON (will be published)
			state = "ON";
		}

		pico_mqtt_destroy_message(message);
		message = NULL;


		/**********************************************************************
		 * PUBLISH SPRINKLER STATE
		 **********************************************************************/
		// Clear buffer
		memset(buffer, 0, sizeof(buffer));
		// Build JSON message
		snprintf(buffer, 128, "{\"Name\":\"%s\",\"Value\":\"%s\"}\n", client_name, state);

		// Create mqtt message
		message = pico_mqtt_create_message(TOPIC_SPR, buffer, (uint32_t)strlen(buffer));

		// Publish on topic
		pico_mqtt_publish(mqtt, message, 1000);

		// Clear the message and internal buffering
		pico_mqtt_destroy_message(message);
		message = NULL;
	}
	pico_mqtt_unsubscribe(mqtt, TOPIC_HUM, 5000 );
	return 0;
}
