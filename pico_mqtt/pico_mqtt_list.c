#include "pico_mqtt_list.h"

#define ERROR -1
#define SUCCES 0

/**
* Private data types
**/

struct element
{
	struct element* previous;
	struct element* next;
	struct pico_mqtt_message* message;
};

struct pico_mqtt_list
{
	uint32_t length;
	struct element* first;
	struct element* last;
};

/**
* Private function prototypes
**/

static inline int create_element(struct element** element, struct pico_mqtt_message* message);
static inline int destroy_element(struct element** element);
static int get_element_by_index(struct pico_mqtt_list* list, struct element** element, uint32_t index);
static int get_element_by_message(struct pico_mqtt_list* list, struct element** element, struct pico_mqtt_message* message);
static int get_element_by_message_id(struct pico_mqtt_list* list, struct element** element, uint16_t  message_id);
static int remove_element(struct pico_mqtt_list* list, struct element* element);

#ifdef DEBUG
static void print_element(struct element* e);
#endif

/**
* Public function declaration
**/

int pico_mqtt_list_create(struct pico_mqtt_list** list_ptr)
{
#ifdef DEBUG
	if(list_ptr == NULL)
	{
		return ERROR;
	}
#endif

	*list_ptr = (struct pico_mqtt_list*) malloc (sizeof(struct pico_mqtt_list));
	if(*list_ptr == NULL)
	{
		return ERROR;
	}

	**list_ptr = (struct pico_mqtt_list) {.length = 0,.first = NULL, .last = NULL};

	return SUCCES;
}

int pico_mqtt_list_push(struct pico_mqtt_list* list, struct pico_mqtt_message* message)
{
	struct element* element = NULL;
#ifdef DEBUG
	if((list == NULL) || (message == NULL))
	{
		return ERROR;
	}
#endif

	if(create_element(&element, message) == ERROR)
	{
		return ERROR;
	}

	list->length += 1;

	if(list->length == 1)
	{
		list->first = element;
		list->last = element;
		return SUCCES;
	}
	

	element->previous = list->last;
	list->last = element;
	element->previous->next = element;
	
	return SUCCES;
}

int pico_mqtt_list_add(struct pico_mqtt_list* list, struct pico_mqtt_message* message, uint32_t index)
{
	struct element* element = NULL;
	struct element* next_element = NULL;

#ifdef DEBUG
	if((list == NULL) || (message == NULL))
	{
		return ERROR;
	}

	/* check if the message is already in the list */
	if(get_element_by_message(list, &element, message) == SUCCES)
	{
		return ERROR;
	}
#endif

	if(index >= list->length)
	{
		return ERROR;
	}

	if(create_element(&element, message) == ERROR)
	{
		return ERROR;
	}

	list->length += 1;
	
	if(list->length == 1)
	{
		list->first = element;
		list->last = element;
		return SUCCES;
	}

	if(get_element_by_index(list, &next_element, index) == ERROR)
	{
		destroy_element(&element);
		return ERROR;
	}

	if(index == 0)
	{
		list->first = element;
	}

	if(index+1 == list->length)
	{
		list->last = element;
	}

	element->previous = next_element->previous;
	element->next = next_element;
	next_element->previous = element;
	if(index != 0)
	{
		element->previous->next = element;
	}
	
	return SUCCES;
}

int pico_mqtt_list_find(struct pico_mqtt_list* list, struct pico_mqtt_message** message_ptr, uint16_t message_id)
{
	struct element* element = NULL;

#ifdef DEBUG
	if((list == NULL) || (message_ptr == NULL))
	{
		return ERROR;
	}
#endif

	if(get_element_by_message_id(list, &element, message_id) == ERROR)
	{
		return ERROR;
	}

	*message_ptr = element->message;
	return SUCCES;
}

int pico_mqtt_list_remove(struct pico_mqtt_list* list, struct pico_mqtt_message* message)
{
	struct element* element = NULL;

#ifdef DEBUG
	if((list == NULL) || (message == NULL))
	{
		return ERROR;
	}
#endif

	if(list->length == 0)
	{
		return ERROR;
	}

	if(get_element_by_message(list, &element, message) == ERROR)
	{
		return ERROR;
	}

	return remove_element(list, element);
}

int pico_mqtt_list_remove_by_index(struct pico_mqtt_list* list, uint32_t index)
{
	struct element* element = NULL;

#ifdef DEBUG
	if(list == NULL)
	{
		return ERROR;
	}
#endif
	if(index >= list->length)
	{
		return ERROR;
	}

	if(get_element_by_index(list, &element, index) == ERROR)
	{
		return ERROR;
	}

	return remove_element(list, element);
}
int pico_mqtt_list_remove_by_id(struct pico_mqtt_list* list, uint16_t message_id)
{
	struct element* element = NULL;

#ifdef DEBUG
	if(list == NULL)
	{
		return ERROR;
	}
#endif

	if(list->length == 0)
	{
		return ERROR;
	}

	if(get_element_by_message_id(list, &element, message_id) == ERROR)
	{
		return ERROR;
	}

	return remove_element(list, element);
}

int pico_mqtt_list_peek(struct pico_mqtt_list* list, struct pico_mqtt_message** message_ptr)
{
#ifdef DEBUG
	if((list == NULL) || (message_ptr == NULL))
	{
		return ERROR;
	}
#endif

	if(list->length == 0)
	{
		*message_ptr = NULL;
		return ERROR;
	}

	*message_ptr = list->first->message;
	return SUCCES;
}

int pico_mqtt_list_pop(struct pico_mqtt_list* list, struct pico_mqtt_message** message_ptr)
{
	struct element* element = NULL;

#ifdef DEBUG
	if((list == NULL) || (message_ptr == NULL))
	{
		return ERROR;
	}
#endif

	if(list->length == 0)
	{
		*message_ptr = NULL;
		return ERROR;
	}

	list->length -= 1;
	element = list->first;
	list->first = element->next;

	if(list->length == 0)
	{
		list->last = NULL;
	} else {
		element->next->previous = NULL;
	}
	
	*message_ptr = element->message;
	return destroy_element(&element);
}

int pico_mqtt_list_length(struct pico_mqtt_list* list, uint32_t* length)
{
#ifdef DEBUG
	if((list == NULL) || (length == NULL))
	{
		return ERROR;
	}
#endif
	*length = list->length;
	return SUCCES;
}

int pico_mqtt_list_destroy(struct pico_mqtt_list** list_ptr)
{
	struct element* current = NULL;
	struct element* next = NULL;
#ifdef DEBUG
	if( list_ptr == NULL )
	{
		return ERROR;
	}
#endif
	
	current = (*list_ptr)->first;
	while((*list_ptr)->length > 0)
	{
		next = current->next;
		if(remove_element(*list_ptr, current) == ERROR)
		{
			return ERROR;
		}
		current = next;
	}
	
	free(*list_ptr);
	*list_ptr = NULL;
	return SUCCES;	
}

/**
* Private function implementation
**/

static inline int create_element(struct element** element, struct pico_mqtt_message* message)
{
#ifdef DEBUG
	if((element == NULL) || (message == NULL))
	{
		return ERROR;
	}
#endif

	*element = (struct element*) malloc(sizeof(struct element));
	
	if(*element == NULL)
	{
		return ERROR;
	}

	(*element)->previous = NULL;
	(*element)->next = NULL;
	(*element)->message = message;

	return SUCCES;
}

static inline int destroy_element(struct element** element)
{
#ifdef DEBUG
	if(element == NULL)
	{
		return ERROR;
	}
#endif

	free(*element);
	*element = NULL;
	return SUCCES;
}

static int get_element_by_index(struct pico_mqtt_list* list, struct element** element, uint32_t index)
{
	uint32_t i = 0;

#ifdef DEBUG
	if((list == NULL) || (element == NULL) || (index >= list->length))
	{
		return ERROR;
	}
#endif

	*element = list->first;

	for(i = 0; i < index; ++i)
	{
		*element = (*element)->next;
	}

	return SUCCES;
}

static int get_element_by_message(struct pico_mqtt_list* list, struct element** element, struct pico_mqtt_message* message)
{
	uint32_t index = 0;

#ifdef DEBUG
	if((list == NULL) || (element == NULL) || (message == NULL))
	{
		return ERROR;
	}
#endif

	*element = list->first;

	for(index = 0; index < list->length; ++index)
	{
		if((*element)->message == message)
		{
			return SUCCES;
		}
		*element = (*element)->next;
	}

	return ERROR;
}

static int get_element_by_message_id(struct pico_mqtt_list* list, struct element** element, uint16_t  message_id)
{
	uint32_t index = 0;

#ifdef DEBUG
	if((list == NULL) || (element == NULL))
	{
		return ERROR;
	}
#endif

	*element = list->first;

	for(index = 0; index < list->length; ++index)
	{
		if((*element)->message->message_id == message_id)
		{
			return SUCCES;
		}
		*element = (*element)->next;
	}

	return ERROR;	
}

static int remove_element(struct pico_mqtt_list* list, struct element* element)
{
#ifdef DEBUG
	if((list == NULL) || (element == NULL))
	{
		return ERROR;
	}
#endif

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
	
	return destroy_element(&element);
}

#ifdef DEBUG

/**
* Debug Function Implementation
**/

static void print_element(struct element* e)
{
	printf("|  +---------------------------+  |\n");
	printf("|  | list element: %10p  |  |\n", e);
	printf("|  | previous: %10p      |  |\n", e->previous);
	printf("|  | next: %10p          |  |\n", e->next);
	printf("|  | message: %10p       |  |\n", e->message);
	printf("|  | message_id: %10d    |  |\n", e->message->message_id);
	printf("|  +---------------------------+  |\n");
}

void print_list(struct pico_mqtt_list* list)
{
	uint32_t i = 0;
	struct element* current = list->first;

	printf("+---------------------------------+\n");
	printf("|  list: %10p               |\n", list);
	printf("|  length: %10d             |\n", list->length);
	printf("|                                 |\n");
	printf("|  first: %10p              |\n", list->first);
	printf("|  last: %10p               |\n", list->last);
	printf("|                                 |\n");
	printf("|                                 |\n");

	while( current != NULL )
	{
		printf("|  index: %10d              |\n", i++);
		print_element( current );
		printf("|                                 |\n");
		current = current->next;
	}

	printf("+---------------------------------+\n");
}

#endif
