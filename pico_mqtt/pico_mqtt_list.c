#include "pico_mqtt_list.h"

/**
* Private data types
**/

struct element
{
	struct element* previous;
	struct element* next;
	struct pico_mqtt_packet* packet;
};

#define ELEMENT_EMPTY (struct element){\
	.previous = NULL,\
	.next = NULL,\
	.packet = NULL,\
}

struct pico_mqtt_list
{
	uint32_t length;
	struct element* first;
	struct element* last;
	int* error;
};

#define PICO_MQTT_LIST_EMPTY (struct pico_mqtt_list){\
	.length = 0,\
	.first = NULL,\
	.last = NULL,\
	.error = NULL\
}

/**
* Private function prototypes
**/

static struct element* create_element(struct pico_mqtt_packet* packet);
static void destroy_element(struct element* element);
static struct element* get_element_by_index(struct pico_mqtt_list* list, uint32_t index);
static int list_add(struct pico_mqtt_list* list, struct pico_mqtt_packet* packet, uint32_t index);
static void remove_element(struct pico_mqtt_list* list, struct element* element);
/*void print_list(struct pico_mqtt_list* list);*/

/**
* Public function declaration
**/

struct pico_mqtt_list* pico_mqtt_list_create( int* error )
{
	struct pico_mqtt_list* list = NULL;

	CHECK_NOT_NULL(error);

	list = (struct pico_mqtt_list*) MALLOC (sizeof(struct pico_mqtt_list));
	if(list == NULL)
			return NULL;

	*list = PICO_MQTT_LIST_EMPTY;
	list->error = error;
	return list;
}

int pico_mqtt_list_push_back(struct pico_mqtt_list* list, struct pico_mqtt_packet* packet)
{
	return list_add( list, packet, list->length);
}


struct pico_mqtt_packet* pico_mqtt_list_get(struct pico_mqtt_list* list, uint16_t packet_id)
{
	uint32_t i = 0;
	struct element* element = NULL;
	struct pico_mqtt_packet* packet = NULL;

	CHECK_NOT_NULL(list);

	if(list->length == 0)
		return NULL;

	element = list->last;

	for(i = list->length; i > 0; --i)
	{
		if(element->packet->packet_id == packet_id)
		{
			packet = element->packet;
			remove_element(list, element);
			return packet;
		}
		element = element->previous;
	}

	return NULL;
}

uint8_t pico_mqtt_list_contains(struct pico_mqtt_list* list, uint16_t packet_id)
{
	struct element* element = NULL;

	CHECK_NOT_NULL(list);

	element = list->first;

	while(element != NULL)
	{
		if(element->packet->packet_id == packet_id)
			return 1;

		element = element->next;
	}

	return 0;
}

struct pico_mqtt_packet* pico_mqtt_list_pop(struct pico_mqtt_list* list)
{
	struct element* element = NULL;
	struct pico_mqtt_packet* packet = NULL;

	CHECK_NOT_NULL(list);
	CHECK(list->length != 0, "Popping an empty list, a stange thing to do.\n");

	if(list->length == 0)
		return NULL;

	list->length -= 1;
	element = list->first;
	list->first = element->next;

	if(list->length == 0)
	{
		list->last = NULL;
	} else {
		element->next->previous = NULL;
	}

	packet = element->packet;
	destroy_element(element);
	return packet;
}

uint32_t pico_mqtt_list_length(struct pico_mqtt_list* list)
{
	CHECK_NOT_NULL(list);

	return list->length;
}

void pico_mqtt_list_destroy(struct pico_mqtt_list* list)
{
	struct element* current = NULL;
	struct element* next = NULL;

	if(list == NULL)
		return;

	current = list->first;
	while(current != NULL)
	{
		next = current->next;
		FREE(current->packet);
		remove_element(list, current);
		current = next;
	}

	FREE(list);
}

/**
* Private function implementation
**/

static struct element* create_element(struct pico_mqtt_packet* packet)
{
	struct element* element = NULL;

	CHECK_NOT_NULL( packet );

	element = (struct element*) MALLOC(sizeof(struct element));

	if(element == NULL)
	{
		PERROR("Unable to allocate the memory for the element.\n");
		return element;
	}

	*element = ELEMENT_EMPTY;
	element->packet = packet;

	return element;
}

static void destroy_element(struct element* element)
{
	CHECK_NOT_NULL( element );
	FREE(element);
}

static struct element* get_element_by_index(struct pico_mqtt_list* list, uint32_t index)
{
	uint32_t i = 0;
	struct element* element = NULL;

	CHECK_NOT_NULL( list );
	CHECK( index < list->length, "The index is not inside the bounds of the list.\n");

	if(index > list->length / 2)
	{
		element = list->last;

		for(i = list->length - 1; i > index; --i)
		{
			element = element->previous;
		}
	} else {
		element = list->first;

		for(i = 0; i < index; ++i)
		{
			element = element->next;
		}
	}

	return element;
}

static int list_add(struct pico_mqtt_list* list, struct pico_mqtt_packet* packet, uint32_t index)
{
	struct element* element = NULL;
	struct element* next_element = NULL;

	CHECK_NOT_NULL(list);
	CHECK_NOT_NULL(packet);
	CHECK(index <= list->length, "The requested index is outside of the lists range.\n");


	element = create_element( packet );
	if(element == NULL)
	{
		PTODO("set the appropriate error.\n");
		return ERROR;
	}

	list->length += 1;

	if(list->length == 1)
	{
		list->first = element;
		list->last = element;
		element->previous = NULL;
		element->next = NULL;
		return SUCCES;
	}

	if(index + 1 == list->length)
	{
		list->last->next = element;
		element->previous = list->last;
		element->next = NULL;
		list->last = element;
		return SUCCES;
	}

	next_element = get_element_by_index(list, index);
	CHECK_NOT_NULL(next_element);

	element->previous = next_element->previous;
	element->next = next_element;
	next_element->previous = element;

	if(index != 0)
	{
		element->previous->next = element;
	} else {
		list->first = element;
	}

	return SUCCES;
}

/*does not remove the data owned */
static void remove_element(struct pico_mqtt_list* list, struct element* element)
{
	CHECK_NOT_NULL(list);
	CHECK_NOT_NULL(element);

	list->length -= 1;

	if(list->first == element)
	{
		list->first = element->next;
	} else {
		element->previous->next = element->next;
	}

	if(list->last == element)
	{
		list->last = element->previous;
	} else {
		element->next->previous = element->previous;
	}

	destroy_element(element);
}

/**
* Debug Function Implementation
**/
/*
static void print_element(struct element* e)
{
	printf("|  +---------------------------+  |\n");
	printf("|  | list element: %10p  |  |\n", e);
	printf("|  | previous: %10p      |  |\n", e->previous);
	printf("|  | next: %10p          |  |\n", e->next);
	printf("|  | packet: %10p       |  |\n", e->packet);
	printf("|  | packet_id: %10d    |  |\n", e->packet->packet_id);
	printf("|  +---------------------------+  |\n");
}

void print_list(struct pico_mqtt_list* list)
{
	uint32_t i = 0;

	printf("+---------------------------------+\n");
	printf("|  list: %10p               |\n", list);
	printf("|  length: %10d             |\n", list->length);
	printf("|                                 |\n");
	printf("|  first: %10p              |\n", list->first);
	printf("|  last: %10p               |\n", list->last);
	printf("|                                 |\n");
	printf("|                                 |\n");

	for(i = 0; i<list->length; i++)
	{
		printf("|  index: %10d              |\n", i);
		print_element( get_element_by_index(list, i) );
		printf("|                                 |\n");
	}

	printf("+---------------------------------+\n");
}
*/
