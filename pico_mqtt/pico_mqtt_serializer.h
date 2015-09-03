#ifndef PICO_MQTT_SERIALIZER_H
#define PICO_MQTT_SERIALIZER_H

#include <stdint.h>
#include "pico_mqtt.h"

int pico_mqtt_serializer_create_connect( 
	struct pico_mqtt* mqtt, 
	struct pico_mqtt_message** return_message, 
	uint16_t keep_alive_time,
	uint8_t retain,
	uint8_t quality_of_service,
	uint8_t clean_session,
	struct pico_mqtt_data* client_id,
	struct pico_mqtt_data* topic,
	struct pico_mqtt_data* message,
	struct pico_mqtt_data* username,
	struct pico_mqtt_data* password
	);

#ifdef DEBUG /* only needed for debug purposes*/
int pico_mqtt_create_connack(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint8_t session_present,
	uint8_t return_code
	);
#endif

int pico_mqtt_create_publish(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** return_message, 
	uint8_t quality_of_service,
	uint8_t retain,
	struct pico_mqtt_data* topic,
	struct pico_mqtt_data* data
	);

int pico_mqtt_create_puback(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	);

int pico_mqtt_create_pubrec(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	);

int pico_mqtt_create_pubrel(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	);

int pico_mqtt_create_pubcomp(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	);

int pico_mqtt_create_subscribe(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	struct pico_mqtt_data* topic,
	uint8_t quality_of_service
	);

#ifdef DEBUG /* only needed for debug purposes*/
int pico_mqtt_create_suback(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packed_id,
	uint8_t return_code
	);
#endif

int pico_mqtt_create_unsubscribe(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	struct pico_mqtt_data* topic
	);

#ifdef DEBUG  /* only needed for debug purposes*/
int pico_mqtt_create_unsuback(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result,
	uint16_t packet_id
	);
#endif

int pico_mqtt_create_pingreq(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result
	);

#ifdef DEBUG  /* only needed for debug purposes*/
int pico_mqtt_create_pingresp(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result
	);
#endif

int pico_mqtt_create_disconnect(
	struct pico_mqtt* mqtt,
	struct pico_mqtt_message** result
	);

#endif
