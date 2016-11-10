#ifndef PICO_MQTT_H
#define PICO_MQTT_H

#include "pico_mqtt_data_types.h"
#include "pico_mqtt_configuration.h"
#include "pico_mqtt_private.h"
#include <stdlib.h>
#include "pico_mqtt_stream.h"
#include "pico_mqtt_serializer.h"
#include "pico_mqtt_list.h"
#include "pico_mqtt_debug.h"
#include "pico_mqtt_list.h"

/* ALL DATA PASSED INTO MQTT OR RETREVED FROM MQTT SHOULD BE FREED BY THE USER */

/**
* Public Function Prototypes
**/

struct pico_mqtt* pico_mqtt_create( void );
void pico_mqtt_destroy(struct pico_mqtt* mqtt);

int pico_mqtt_set_client_id(struct pico_mqtt* mqtt, const char* client_id);
void pico_mqtt_unset_client_id( struct pico_mqtt* mqtt);

int pico_mqtt_set_will_message(struct pico_mqtt* mqtt, const struct pico_mqtt_message* will_message);
void pico_mqtt_unset_will_message( struct pico_mqtt* mqtt);

int pico_mqtt_set_username(struct pico_mqtt* mqtt, const char* username);
void pico_mqtt_unset_username( struct pico_mqtt* mqtt);

int pico_mqtt_set_password(struct pico_mqtt* mqtt, const char* password);
void pico_mqtt_unset_password( struct pico_mqtt* mqtt);

void pico_mqtt_set_keep_alive_time(struct pico_mqtt* mqtt, const uint16_t keep_alive_time);

int pico_mqtt_connect(struct pico_mqtt* mqtt, const char* uri, const char* port, const uint32_t timeout);
int pico_mqtt_disconnect(struct pico_mqtt* mqtt);

int pico_mqtt_ping(struct pico_mqtt* mqtt, const uint32_t timeout );

int pico_mqtt_publish(struct pico_mqtt* mqtt, struct pico_mqtt_message* message, const uint32_t timeout);
int pico_mqtt_receive(struct pico_mqtt* mqtt, struct pico_mqtt_message** message, const uint32_t timeout);

int pico_mqtt_subscribe(struct pico_mqtt* mqtt, const char* topic, const uint8_t quality_of_service, const uint32_t timeout);
int pico_mqtt_unsubscribe(struct pico_mqtt* mqtt, const char* topic, const uint32_t timeout);

/**
* Helper Function Prototypes
**/

struct pico_mqtt_data* pico_mqtt_string_to_data(const char* string);
struct pico_mqtt_message* pico_mqtt_create_message(const char* topic, const void* data, const uint32_t length);
struct pico_mqtt_message* pico_mqtt_copy_message( const struct pico_mqtt_message* message);
void pico_mqtt_destroy_message(struct pico_mqtt_message * message);
void pico_mqtt_destroy_data(struct pico_mqtt_data * data);

struct pico_mqtt_data* pico_mqtt_create_data(const void* data, const uint32_t length);
struct pico_mqtt_data* pico_mqtt_copy_data( const struct pico_mqtt_data* data);

void pico_mqtt_message_set_quality_of_service(struct pico_mqtt_message* message, uint8_t quality_of_service);
uint8_t pico_mqtt_message_get_quality_of_service(struct pico_mqtt_message* message);

void pico_mqtt_message_set_retain(struct pico_mqtt_message* message, uint8_t retain_flag);
uint8_t pico_mqtt_message_get_retain(struct pico_mqtt_message* message);

uint8_t pico_mqtt_message_is_duplicate_flag_set(struct pico_mqtt_message* message);
uint16_t pico_mqtt_message_get_message_id(struct pico_mqtt_message* message);
char* pico_mqtt_message_get_topic(struct pico_mqtt_message* message);

const char* pico_mqtt_get_protocol_version( void );

#endif