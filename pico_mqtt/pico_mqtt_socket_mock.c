#include "pico_mqtt_socket.h"
#include "pico_mqtt_socket_mock.h"

struct pico_mqtt_socket{
  int* error;
};

struct socket_mock{
  uint8_t fail_flag;
  int error_value;
  int return_value;
  uint32_t bytes_to_receive;
  uint32_t bytes_to_send;
  uint64_t time;
  uint64_t time_interval;
};

#define SOCKET_MOCK_EMPTY (struct socket_mock){\
  .fail_flag = 0,\
  .error_value = 0,\
  .return_value = 0,\
  .bytes_to_receive = 0,\
  .bytes_to_send = 0,\
  .time = 0,\
  .time_interval = 0,\
}

static struct socket_mock socket_mock = SOCKET_MOCK_EMPTY;

/**
* mock function implementation
**/

/* create pico mqtt socket */
struct pico_mqtt_socket* pico_mqtt_connection_create( int* error )
{
  struct pico_mqtt_socket* socket = NULL;

  if(socket_mock.fail_flag)
    return NULL;

  socket = (struct pico_mqtt_socket*) MALLOC (sizeof(struct pico_mqtt_socket));
  socket->error = error;
  *error = socket_mock.error_value;
  return socket;
}

/* connect to the URI*/
int pico_mqtt_connection_open(struct pico_mqtt_socket* socket, const char* uri, const char* port)
{
  CHECK_NOT_NULL(socket);
  uri++;
  port++;
  *(socket->error) = socket_mock.error_value;
  return socket_mock.return_value;
}

int pico_mqtt_connection_send_receive(
  struct pico_mqtt_socket* socket,
  struct pico_mqtt_data* send_buffer,
  struct pico_mqtt_data* receive_buffer,
  uint64_t time_left
)
{
  uint32_t i = 0;
  CHECK_NOT_NULL(socket);
  time_left++;
  *(socket->error) = socket_mock.error_value;

  if(send_buffer != NULL)
  {
    if(send_buffer->length < socket_mock.bytes_to_send)
    {
      send_buffer->data = (uint8_t*) (send_buffer->data) + socket_mock.bytes_to_send;
      send_buffer->length -= socket_mock.bytes_to_send;
    } else {
      send_buffer->data = (uint8_t*) (send_buffer->data) + send_buffer->length;
      send_buffer->length = 0;
    }
  }

  if(receive_buffer != NULL)
  {
    if(receive_buffer->length < socket_mock.bytes_to_receive)
    {
      socket_mock.bytes_to_receive = receive_buffer->length;
    }

    for(i=0; i<socket_mock.bytes_to_receive; ++i)
    {
      *((uint8_t*) receive_buffer->data) = (uint8_t) i;
      receive_buffer->data = ((uint8_t*) receive_buffer->data) + 1;
      --(receive_buffer->length);
    }

    receive_buffer->data = (uint8_t*) (receive_buffer->data) + socket_mock.bytes_to_receive;
  }

  return socket_mock.return_value;
}

void pico_mqtt_connection_destroy( struct pico_mqtt_socket* socket)
{
  *(socket->error) = socket_mock.error_value;
  FREE(socket);
}

/* get current time in miliseconds */
uint64_t get_current_time( void  )
{
  uint64_t time = socket_mock.time;
  socket_mock.time += socket_mock.time_interval;
  return time;
}


/**
* Debug function implementation
**/

void socket_mock_reset( void )
{
    socket_mock = SOCKET_MOCK_EMPTY;
}

void socket_mock_set_create_fail( uint8_t result )
{
  socket_mock.fail_flag = result;
}

void socket_mock_set_error_value( int error_value )
{
  socket_mock.error_value = error_value;
}

void socket_mock_set_return_value( int return_value )
{
  socket_mock.return_value = return_value;
}

void socket_mock_set_bytes_to_receive( uint32_t bytes_to_receive )
{
  socket_mock.bytes_to_receive = bytes_to_receive;
}

void socket_mock_set_bytes_to_send( uint32_t bytes_to_send )
{
  socket_mock.bytes_to_send = bytes_to_send;
}

void socket_mock_set_time( uint64_t time )
{
  socket_mock.time = time;
}

void socket_mock_set_time_interval( uint64_t time )
{
  socket_mock.time_interval = time;
}