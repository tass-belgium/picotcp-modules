#ifndef PICO_MQTT_LIST_H
#define PICO_MQTT_LIST_H

#include <stdint.h>
#include <stdlib.h>
#include "pico_mqtt.h"

struct pico_mqtt_list;

int pico_mqtt_list_create(struct pico_mqtt_list** list_ptr);

int pico_mqtt_list_push(struct pico_mqtt_list* list, struct pico_mqtt_message* message);

int pico_mqtt_list_add(struct pico_mqtt_list* list, struct pico_mqtt_message* message, uint32_t index);

int pico_mqtt_list_find(struct pico_mqtt_list* list, struct pico_mqtt_message** message_ptr, uint16_t message_id);

int pico_mqtt_list_remove(struct pico_mqtt_list* list, struct pico_mqtt_message* message);

int pico_mqtt_list_remove_by_index(struct pico_mqtt_list* list, uint32_t index);

int pico_mqtt_list_remove_by_id(struct pico_mqtt_list* list, uint16_t message_id);

int pico_mqtt_list_peek(struct pico_mqtt_list* list, struct pico_mqtt_message** message_ptr);

int pico_mqtt_list_pop(struct pico_mqtt_list* list, struct pico_mqtt_message** message_ptr);

int pico_mqtt_list_length(struct pico_mqtt_list* list, uint32_t* length);

int pico_mqtt_list_destroy(struct pico_mqtt_list** list_ptr);

#endif
