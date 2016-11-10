#include "pico_mqtt.h"

int main()
{
	struct pico_mqtt* mqtt = NULL;
	struct pico_mqtt_message* message = NULL;

	printf("\nCreating the mqtt object\n");
	mqtt = pico_mqtt_create();

	printf("\nConnecting the client\n");
	pico_mqtt_set_client_id(mqtt, "1");
	pico_mqtt_connect(mqtt, "localhost", "1883", 1000);

	printf("\nSubscribing to test\n");
	pico_mqtt_subscribe(mqtt, "test", 0, 1000);

/*	printf("\nReceive a message\n");
	pico_mqtt_receive(mqtt, &message, 1000000);
	printf("Received message: %s\n", (char*) message->data->data);
	pico_mqtt_destroy_message(message);*/

	printf("\nSending a message\n");
	message = pico_mqtt_create_message("test2", "Hello world!", sizeof("Hello world!"));
	pico_mqtt_message_set_quality_of_service(message, 1);
	pico_mqtt_publish(mqtt, message, 1000);
	pico_mqtt_destroy_message(message);

	printf("\nDisconnecting the client\n");
	pico_mqtt_disconnect(mqtt);

	printf("\nDestroying the object\n");
	pico_mqtt_destroy(mqtt);

	printf("\nDone!\n");
	return 0;
}

/*
	struct pico_mqtt* mqtt1 = NULL;
	struct pico_mqtt* mqtt2 = NULL;
	struct pico_mqtt_message* message = NULL;

	mqtt1 = pico_mqtt_create();
	mqtt2 = pico_mqtt_create();

	pico_mqtt_set_client_id(mqtt1, "1");
	pico_mqtt_set_client_id(mqtt2, "2");

	pico_mqtt_connect(mqtt1, "localhost", "1883", 1000);
	pico_mqtt_connect(mqtt2, "localhost", "1883", 1000);

	pico_mqtt_ping(mqtt1, 1000);
	pico_mqtt_ping(mqtt2, 1000);

	pico_mqtt_subscribe(mqtt1, "test", 0, 1000);	

	message = pico_mqtt_create_message("test", "Hello world!", sizeof("Hello world!"));
	pico_mqtt_message_set_quality_of_service(message, 0);
	pico_mqtt_publish(mqtt2, message, 1000);
	pico_mqtt_destroy_message(message);

	pico_mqtt_receive(mqtt1, &message, 1000);
	printf("received message: %s\n", (char*)message->data);
	pico_mqtt_destroy_message(message);

	pico_mqtt_disconnect(mqtt1);
	pico_mqtt_disconnect(mqtt2);

	pico_mqtt_destroy(mqtt1);
	pico_mqtt_destroy(mqtt2);

	CHECK_NO_ALLOCATIONS();
*/