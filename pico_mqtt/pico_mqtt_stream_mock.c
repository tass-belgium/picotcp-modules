#include "pico_mqtt_stream.h"
#include "pico_mqtt_stream_mock.h"

struct pico_mqtt_stream{
  int* error;
};

struct stream_mock{
  struct pico_mqtt_data return_data;
  uint8_t fail_flag;
  int error_value;
  int return_value;
};

#define STREAM_MOCK_EMPTY (struct stream_mock){\
  .fail_flag = 0,\
  .error_value = 0,\
  .return_value = 0,\
}

static struct stream_mock stream_mock = STREAM_MOCK_EMPTY;

/**
* mock function implementation
**/

/* create pico mqtt stream */
struct pico_mqtt_stream* pico_mqtt_stream_create( int* error )
{
  struct pico_mqtt_stream* stream = NULL;
  CHECK_NOT_NULL(error);

  if(stream_mock.fail_flag)
    return NULL;

  stream = (struct pico_mqtt_stream*) MALLOC (sizeof(struct pico_mqtt_stream));
  stream->error = error;
  *error = stream_mock.error_value;
  return stream;
}

/* close and free pico mqtt stream*/
void pico_mqtt_stream_destroy( struct pico_mqtt_stream* stream)
{
  if(stream == NULL)
    return;

  *(stream->error) = stream_mock.error_value;
  FREE(stream);
}


int pico_mqtt_stream_connect( struct pico_mqtt_stream* stream, const char* uri, const char* port)
{
    uri++;
    port++;
    *(stream->error) = stream_mock.error_value;
    return stream_mock.return_value;
}

uint8_t pico_mqtt_stream_is_message_sending( const struct pico_mqtt_stream* stream )
{
    *(stream->error) = stream_mock.error_value;
    return (uint8_t) stream_mock.return_value;
}

uint8_t pico_mqtt_stream_is_message_received( const struct pico_mqtt_stream* stream )
{
    *(stream->error) = stream_mock.error_value;
    return (uint8_t)stream_mock.return_value;
}

void pico_mqtt_stream_set_send_message( struct pico_mqtt_stream* stream, struct pico_mqtt_data message)
{
    message.length++;
    *(stream->error) = stream_mock.error_value;
    return;
}

struct pico_mqtt_data pico_mqtt_stream_get_received_message( struct pico_mqtt_stream* stream )
{
    *(stream->error) = stream_mock.error_value;
    return stream_mock.return_data;
}

int pico_mqtt_stream_send_receive( struct pico_mqtt_stream* stream, uint64_t* time_left, uint8_t wait_for_input)
{
    time_left++;
    wait_for_input++;
    *(stream->error) = stream_mock.error_value;
    return stream_mock.return_value;
}

/**
* Debug function implementation
**/

void stream_mock_reset( void )
{
    stream_mock = STREAM_MOCK_EMPTY;
}

void stream_mock_set_create_fail( uint8_t result )
{
  stream_mock.fail_flag = result;
}

void stream_mock_set_error_value( int error_value )
{
  stream_mock.error_value = error_value;
}

void stream_mock_set_return_value( int return_value )
{
  stream_mock.return_value = return_value;
}

void stream_mock_set_return_data( struct pico_mqtt_data data )
{
  stream_mock.return_data = data;
}