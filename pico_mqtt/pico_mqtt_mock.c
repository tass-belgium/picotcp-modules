#include "pico_mqtt.h"
#include "pico_mqtt_mock.h"

struct pico_mqtt_{
  int* error;
};

struct mock{
  struct pico_mqtt_data return_data;
  uint8_t fail_flag;
  int error_value;
  int return_value;
};

#define MOCK_EMPTY (struct mock){\
  .fail_flag = 0,\
  .error_value = 0,\
  .return_value = 0,\
}

static struct mock mock = MOCK_EMPTY;

/**
* mock function implementation
**/


/**
* Debug function implementation
**/

void mock_reset( void )
{
    mock = MOCK_EMPTY;
}

void mock_set_create_fail( uint8_t result )
{
  mock.fail_flag = result;
}

void mock_set_error_value( int error_value )
{
  mock.error_value = error_value;
}

void mock_set_return_value( int return_value )
{
  mock.return_value = return_value;
}