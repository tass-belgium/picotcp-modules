#ifndef PICO_MQTT_H
#define PICO_MQTT_H

#include "pico_mqtt_data_types.h"
#include "pico_mqtt_configuration.h"
#include "pico_mqtt_private.h"
#include <sys/time.h>
#include <stdlib.h>
#include "pico_mqtt_stream.h"
#include "pico_mqtt_serializer.h"

#if ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1
#include "pico_mqtt_list.h"
#endif /* ENABLE_QUALITY_OF_SERVICE_1_AND_2 == 1 */



#if 1== 0
/**
* Public Function Prototypes
**/

struct pico_mqtt* pico_mqtt_create( void );
void pico_mqtt_destroy(struct pico_mqtt* mqtt);

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

int pico_mqtt_connect(struct pico_mqtt* mqtt, const char* uri, const char* port, const uint32_t timeout);
int pico_mqtt_disconnect(struct pico_mqtt* mqtt);

int pico_mqtt_restart(struct pico_mqtt* mqtt, const uint32_t timeout); /* optional function */
int pico_mqtt_ping(struct pico_mqtt* mqtt, const uint32_t timeout );

int pico_mqtt_receive(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, const uint32_t timeout);
int pico_mqtt_publish(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, const uint32_t timeout);

int pico_mqtt_subscribe(struct pico_mqtt* mqtt, const char* topic, const uint8_t quality_of_service, const uint32_t timeout);
int pico_mqtt_unsubscribe(struct pico_mqtt* mqtt, const char* topic, const uint32_t timeout);

/**
* Helper Function Prototypes
**/

void pico_mqtt_destroy_message(struct pico_mqtt_message * message);
void pico_mqtt_destroy_data(struct pico_mqtt_data * data);

struct pico_mqtt_data* pico_mqtt_create_data(const void* data, const uint32_t length);
struct pico_mqtt_data* pico_mqtt_string_to_data(const char* string);
struct pico_mqtt_message pico_mqtt_create_message(const char* topic, struct pico_mqtt_data data);

int pico_mqtt_message_set_quality_of_service(struct pico_mqtt_message* message, int quality_of_service);
int pico_mqtt_message_get_quality_of_service(struct pico_mqtt_message* message);

int pico_mqtt_message_set_retain(struct pico_mqtt_message* message, int quality_of_service);
int pico_mqtt_message_get_retain(struct pico_mqtt_message* message);

int pico_mqtt_message_is_duplicate_flag_set(struct pico_mqtt_message* message);
char* pico_mqtt_message_get_topic(struct pico_mqtt_message* message);


char* pico_mqtt_get_normative_error( const struct pico_mqtt* mqtt);
uint32_t pico_mqtt_get_error_documentation_line( const struct pico_mqtt* mqtt);

const char* pico_mqtt_get_protocol_version( void );

#endif
#endif
