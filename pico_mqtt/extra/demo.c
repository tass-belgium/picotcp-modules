#include "pico_mqtt.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
	printf("Starting the demo\n");

	struct pico_mqtt* mqtt = pico_mqtt_create();

	if(pico_mqtt_set_client_id( mqtt, "12345"))
	{
		printf("An error occured while setting the client_id\n");
	}

	if(pico_mqtt_set_username( mqtt, "my_name"))
	{
		printf("An error occured while setting the username\n");
	}

	if(pico_mqtt_set_password( mqtt, "secret"))
	{
		printf("An error occured while setting the password\n");
	}

	if(pico_mqtt_connect(mqtt,"localhost", "1883", 10))
	{
		printf("An error occured during connecting\n");
	}

	sleep(5);

	pico_mqtt_destroy( mqtt );

	return 0;
}