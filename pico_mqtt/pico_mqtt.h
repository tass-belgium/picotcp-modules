#ifndef PICO_MQTT_H
#define PICO_MQTT_H

#include "pico_mqtt_configuration.h"
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>

/**
* Data Structures
**/

struct pico_mqtt_data
{
	uint32_t length;
	void * data;
};

struct pico_mqtt_message
{
	uint8_t header;
	uint8_t status;
	uint16_t message_id;
	struct pico_mqtt_data topic;
	struct pico_mqtt_data data;
};

#include "pico_mqtt_stream.h"
#include "pico_mqtt_serializer.h"

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1
#include "pico_mqtt_list.h"
#endif /* ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1 */

/**
* Public Function Prototypes
**/

/* allocate memory for the pico_mqtt object and free it */
struct pico_mqtt* pico_mqtt_create( void );
void pico_mqtt_destroy(struct pico_mqtt* mqtt);

/* getters and setters for the connection options, resenable defaults will be used */
int pico_mqtt_set_client_id(struct pico_mqtt* mqtt, const char* client_id);
int pico_mqtt_is_client_id_set(struct pico_mqtt* mqtt);
struct pico_mqtt_data* pico_mqtt_get_client_id(struct pico_mqtt* mqtt);

int pico_mqtt_set_will_message(struct pico_mqtt* mqtt, const struct pico_mqtt_message will_message);
int pico_mqtt_is_will_message_set(struct pico_mqtt* mqtt);
struct pico_mqtt_data* pico_mqtt_get_will_message(struct pico_mqtt* mqtt);

int pico_mqtt_set_username(struct pico_mqtt* mqtt, const char* username);
int pico_mqtt_is_username_set(struct pico_mqtt* mqtt);
struct pico_mqtt_data* pico_mqtt_get_username(struct pico_mqtt* mqtt);

int pico_mqtt_set_password(struct pico_mqtt* mqtt, const char* password);
int pico_mqtt_is_password_set(struct pico_mqtt* mqtt);
struct pico_mqtt_data* pico_mqtt_get_password(struct pico_mqtt* mqtt);

/* connect to the server and disconnect again */
int pico_mqtt_connect(struct pico_mqtt* mqtt, const char* uri, const char* port, const uint32_t timeout);
int pico_mqtt_disconnect(struct pico_mqtt* mqtt);

/* ping the server and flush buffer and subscribtions (back to state after create) */
int pico_mqtt_restart(struct pico_mqtt* mqtt, const uint32_t timeout); /* optional function */
int pico_mqtt_ping(struct pico_mqtt* mqtt, const uint32_t timeout );

/* receive a message or publis a message */
int pico_mqtt_receive(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, const uint32_t timeout);
int pico_mqtt_publish(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, const uint32_t timeout);

/* subscribe to a topic (or multiple topics) and unsubscribe */
int pico_mqtt_subscribe(struct pico_mqtt* mqtt, const char* topic, const uint8_t quality_of_service, const uint32_t timeout);
int pico_mqtt_unsubscribe(struct pico_mqtt* mqtt, const char* topic, const uint32_t timeout);

/**
* Helper Function Prototypes
**/

/* free custom data types */
void pico_mqtt_destroy_message(struct pico_mqtt_message * message);
void pico_mqtt_destroy_data(struct pico_mqtt_data * data);

/* create and destroy custom data types */
struct pico_mqtt_data* pico_mqtt_create_data(const void* data, const uint32_t length);
struct pico_mqtt_data* pico_mqtt_string_to_data(const char* string);
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
const char* pico_mqtt_get_protocol_version( void );

#endif
