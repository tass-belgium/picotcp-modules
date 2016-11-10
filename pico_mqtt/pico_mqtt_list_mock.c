#include "pico_mqtt_list.h"
#include "pico_mqtt_list_mock.h"

struct pico_mqtt_list{
  int* error;
};

struct list_mock{
  struct pico_mqtt_data return_data;
  struct pico_mqtt_packet* packet;
  uint32_t length;
  uint8_t fail_flag;
  int error_value;
  int return_value;
};

#define LIST_MOCK_EMPTY (struct list_mock){\
  .fail_flag = 0,\
  .error_value = 0,\
  .return_value = 0,\
  .length = 0,\
}

static struct list_mock list_mock = LIST_MOCK_EMPTY;

/**
* mock function implementation
**/

/* create pico mqtt list */
struct pico_mqtt_list* pico_mqtt_list_create( int* error )
{
  struct pico_mqtt_list* list = NULL;
  CHECK_NOT_NULL(error);

  if(list_mock.fail_flag)
    return NULL;

  list = (struct pico_mqtt_list*) MALLOC (sizeof(struct pico_mqtt_list));
  list->error = error;
  *error = list_mock.error_value;
  return list;
}

/* close and free pico mqtt list*/
void pico_mqtt_list_destroy( struct pico_mqtt_list* list)
{
  if(list == NULL)
    return;

  *(list->error) = list_mock.error_value;
  FREE(list);
}

int pico_mqtt_list_push_back(struct pico_mqtt_list* list, struct pico_mqtt_packet* packet)
{
  packet++;
  *(list->error) = list_mock.error_value;
  return list_mock.return_value;
}

struct pico_mqtt_packet* pico_mqtt_list_get(struct pico_mqtt_list* list, uint16_t message_id)
{
  message_id++;
  *(list->error) = list_mock.error_value;
  return list_mock.packet;
}

uint8_t pico_mqtt_list_contains(struct pico_mqtt_list* list, uint16_t packet_id)
{
  packet_id++;
  *(list->error) = list_mock.error_value;
  return (uint8_t) list_mock.return_value;
}

struct pico_mqtt_packet* pico_mqtt_list_pop(struct pico_mqtt_list* list)
{
  *(list->error) = list_mock.error_value;
  return list_mock.packet;
}

uint32_t pico_mqtt_list_length(struct pico_mqtt_list* list)
{
  *(list->error) = list_mock.error_value;
  return list_mock.length;
}

/**
* Debug function implementation
**/

void list_mock_reset( void )
{
    list_mock = LIST_MOCK_EMPTY;
}

void list_mock_set_create_fail( uint8_t result )
{
  list_mock.fail_flag = result;
}

void list_mock_set_error_value( int error_value )
{
  list_mock.error_value = error_value;
}

void list_mock_set_return_value( int return_value )
{
  list_mock.return_value = return_value;
}

void list_mock_set_length( uint32_t length )
{
  list_mock.length = length;
}

void list_mock_set_packet( struct pico_mqtt_packet* packet )
{
  list_mock.packet = packet;
}