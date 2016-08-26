#include "pico_mqtt.h"
#include <stdio.h>
#include <stdlib.h>

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

 static void water_garden( uint16_t minutes );
int main_humidity_sensor( void );
int main_better_humidity_sensor( void );
int main_sprinkler( void );


 static void water_garden( uint16_t minutes )
 {
 	printf("Watering the garden for %d minutes\n", minutes);
 }


/**
* Inefficient but simplest implementation
**/

int main_humidity_sensor( void )
{
	struct pico_mqtt_message* message = NULL;
	uint8_t humidity = 0;

	struct pico_mqtt* mqtt = pico_mqtt_create();
	pico_mqtt_connect(mqtt, "www.mygarden.be", "1883", 1000);

	while(1)
	{
		humidity = (uint8_t)(rand() % 100);
		message = pico_mqtt_create_message("garden/humidity", &humidity, 1);
		pico_mqtt_message_set_quality_of_service(message, 1);
		pico_mqtt_publish(mqtt, message, 1000);
		pico_mqtt_destroy_message(message);
	}

	pico_mqtt_disconnect(mqtt);
	pico_mqtt_destroy(mqtt);
	return 0;
}

/**
* More efficient implementation:
*
* No memory allocations are made for each message (internally the data is copied
* for sending). The Quality of Service has only to be set once.
**/

int main_better_humidity_sensor( void )
{
	struct pico_mqtt_message message = PICO_MQTT_MESSAGE_EMPTY;
	struct pico_mqtt_data data = PICO_MQTT_DATA_EMPTY;
	struct pico_mqtt_data topic = PICO_MQTT_DATA_EMPTY;
	struct pico_mqtt* mqtt = NULL;
	char topic_string[] = "garden/humidity";
	uint8_t humidity = 0;

	pico_mqtt_message_set_quality_of_service(&message, 1);

	data.data = &humidity;
	data.length = sizeof(humidity);
	message.data = &data;

	topic.data = topic_string;
	topic.length = sizeof("garden/humidity") - 1; //don't include NULL termination
	message.topic = &topic;

	mqtt = pico_mqtt_create();
	pico_mqtt_connect(mqtt, "www.mygarden.be", "1883", 1000);

	while(1)
	{
		humidity = (uint8_t)(rand() % 100);
		pico_mqtt_publish(mqtt, &message, 1000);
	}

	pico_mqtt_disconnect(mqtt);
	pico_mqtt_destroy(mqtt);
	return 0;
}

/**
* sprinkler implementation
**/

int main_sprinkler( void )
{
	struct pico_mqtt_message* message = NULL;

	struct pico_mqtt* mqtt = pico_mqtt_create();
	pico_mqtt_connect(mqtt, "www.mygarden.be", "1883", 1000);

	pico_mqtt_subscribe(mqtt, "garden/humidity", 1, 1000);

	pico_mqtt_receive(mqtt, &message, 60000); // wait 1 minute for a message
	if(message != NULL)
	{
		water_garden(*((uint8_t*) message->data->data ));
		pico_mqtt_destroy_message(message);
		message = pico_mqtt_create_message("garden/sprinkler", "on", 2);
		pico_mqtt_publish(mqtt, message, 1000);
		pico_mqtt_destroy_message(message);
		message = NULL;
	}

	pico_mqtt_unsubscribe(mqtt, "garden/humidity", 1000 );
	pico_mqtt_disconnect(mqtt);
	pico_mqtt_destroy(mqtt);
	return 0;
}