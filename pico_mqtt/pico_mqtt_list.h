#ifndef PICO_MQTT_LIST_H
#define PICO_MQTT_LIST_H

#include <stdlib.h>
#include "pico_mqtt_data_types.h"
#include "pico_mqtt_private.h"
#include "pico_mqtt_debug.h"

struct pico_mqtt_list;

/* check search direction, should start from oldests element */

struct pico_mqtt_list* pico_mqtt_list_create( int* error );
void pico_mqtt_list_destroy(struct pico_mqtt_list* list);
int pico_mqtt_list_push_back(struct pico_mqtt_list* list, struct pico_mqtt_packet* packet);
struct pico_mqtt_packet* pico_mqtt_list_get(struct pico_mqtt_list* list, uint16_t message_id);
uint8_t pico_mqtt_list_contains(struct pico_mqtt_list* list, uint16_t packet_id);
struct pico_mqtt_packet* pico_mqtt_list_pop(struct pico_mqtt_list* list);
uint32_t pico_mqtt_list_length(struct pico_mqtt_list* list);

#endif