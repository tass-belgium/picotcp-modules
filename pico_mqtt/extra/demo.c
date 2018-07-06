#include "pico_mqtt.h"
#include <stdio.h>
#include <stdlib.h>

// CONFIGURATIONS
#define HOSTNAME 	"localhost" 		// Hostname used by MQTT server in the network: e.g. "10.222.23.157"
#define PORT 		"1883"				// Port     used by MQTT server

#define USER 		"IOT"				// Credentials needed to connect with the MQTT broker
#define PWD  		"fundamentals"		// Credentials needed to connect with the MQTT broker

//include functions from ../garden_watering_example.c
extern int main_humidity_sensor( struct pico_mqtt* mqtt, char* client_name);
extern int main_sprinkler( struct pico_mqtt* mqtt, char* client_name );

typedef enum program_t {
	program_humidity_sensor,
	program_sprinkler,
	program_invalid = 0xff
} program_t;

void help(char* progName)
{
	printf("Usage:\n");
	printf("======\n");
	printf("To publish data\t%s publish\n", progName);
	printf("To consume data\t%s subscribe\n", progName);
}


/* @brief: the start of the MQTT client program
 * @usage: To publish "garden/humidity" data: ./PROG_NAME publish
 *         To consume "garden/humidity" data: ./PROG_NAME subscribe
 */
int main(int argc, char* argv[])
{
	program_t activeProgram = program_invalid;

	// Parse program arguments
	if (argc != 2)
	{
		help(argv[0]);
		return -1;
	}

	if (strcmp(argv[1],"publish") == 0)
	{
		activeProgram = program_humidity_sensor;
	}
	else if (strcmp(argv[1], "subscribe") == 0)
	{
		activeProgram = program_sprinkler;
	}
	else
	{
		help(argv[0]);
		return -1;
	}

	printf("Starting the MQTT client\n");

	struct pico_mqtt* mqtt = pico_mqtt_create();

	char* clientId = argv[1];

	printf("clientId: %s\n", clientId);

	if(pico_mqtt_set_client_id( mqtt, clientId))
	{
		printf("An error occured while setting the client_id\n");
	}

	if(pico_mqtt_set_username( mqtt, USER))
	{
		/*Needed to connect to the MQTT server*/
		// TODO Set up a mosquitto server (sudo apt-get install mosquitto)
		//      Configure the user via mosquitto_passwd -c /etc/mosquitto/pwdFile IOT
		printf("An error occured while setting the username\n");
	}

	if(pico_mqtt_set_password( mqtt, PWD))
	{
		//      Configure the password via mosquitto_passwd -c /etc/mosquitto/pwdFile IOT
		//      password: fundamentals
		printf("An error occured while setting the password\n");
	}

	if(pico_mqtt_connect(mqtt, HOSTNAME, PORT, 10))
	{
		printf("An error occured during connecting\n");
	}


	switch(activeProgram)
	{
	case program_humidity_sensor:
		printf("Starting the humidity sensor\n");
		//This will start an endless loop
		main_humidity_sensor( mqtt, clientId );
		break;
	case program_sprinkler:
		printf("Starting the sprinkler\n");
		// This will read humidity sensor data, print it and publish sprinkler data
		main_sprinkler( mqtt, clientId );
		break;
	default:
		// An error occurred
		break;
	}

	pico_mqtt_disconnect( mqtt );
	pico_mqtt_destroy( mqtt );

	return 0;
}
