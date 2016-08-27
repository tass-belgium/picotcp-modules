#include "pico_mqtt_serializer.h"
#include "pico_mqtt_serializer_mock.h"

struct pico_mqtt_serializer{
  int* error;
};

struct serializer_mock{
	struct pico_mqtt_data return_data;
	struct pico_mqtt_packet* packet;
	uint8_t fail_flag;
	int error_value;
	int return_value;
	uint32_t length;
};

#define SERIALIZER_MOCK_EMPTY (struct serializer_mock){\
  .fail_flag = 0,\
  .error_value = 0,\
  .return_value = 0,\
  .length = 0,\
}

static struct serializer_mock serializer_mock = SERIALIZER_MOCK_EMPTY;

/**
* mock function implementation
**/

/* create pico mqtt serializer */
struct pico_mqtt_serializer* pico_mqtt_serializer_create( int* error )
{
  struct pico_mqtt_serializer* serializer = NULL;
  CHECK_NOT_NULL(error);

  if(serializer_mock.fail_flag)
    return NULL;

  serializer = (struct pico_mqtt_serializer*) MALLOC (sizeof(struct pico_mqtt_serializer));
  serializer->error = error;
  *error = serializer_mock.error_value;
  return serializer;
}

void pico_mqtt_serializer_clear( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);
	*serializer->error = serializer_mock.error_value;
}

/* close and free pico mqtt serializer*/
void pico_mqtt_serializer_destroy( struct pico_mqtt_serializer* serializer)
{
	if(serializer == NULL)
		return;

	*(serializer->error) = serializer_mock.error_value;
	FREE(serializer);
}

struct pico_mqtt_data* pico_mqtt_serialize_length( struct pico_mqtt_serializer* serializer, uint32_t length )
{
	CHECK_NOT_NULL(serializer);
	length++;
	*(serializer->error) = serializer_mock.error_value;
	return &serializer_mock.return_data;
}

int pico_mqtt_deserialize_length( int* error, void* length_void, uint32_t* result)
{
	CHECK_NOT_NULL(error);
	CHECK_NOT_NULL(result);
	CHECK_NOT_NULL(length_void);

	length_void++;
	*error = serializer_mock.error_value;
	*result = serializer_mock.length;
	return serializer_mock.return_value;
}

void pico_mqtt_serializer_set_message_type( struct pico_mqtt_serializer* serializer, uint8_t message_type )
{
	CHECK_NOT_NULL(serializer);
	message_type++;

	*serializer->error = serializer_mock.error_value;
}


void pico_mqtt_serializer_set_client_id( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* client_id)
{
	CHECK_NOT_NULL(serializer);
	client_id++;

	*serializer->error = serializer_mock.error_value;
}

void pico_mqtt_serializer_set_username( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* username)
{
	CHECK_NOT_NULL(serializer);
	username++;

	*serializer->error = serializer_mock.error_value;
}

void pico_mqtt_serializer_set_password( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* password)
{
	CHECK_NOT_NULL(serializer);
	password++;

	*serializer->error = serializer_mock.error_value;
}

void pico_mqtt_serializer_set_message( struct pico_mqtt_serializer* serializer, struct pico_mqtt_message* message)
{
	CHECK_NOT_NULL(serializer);
	message++;

	*serializer->error = serializer_mock.error_value;
}

void pico_mqtt_serializer_set_keep_alive_time( struct pico_mqtt_serializer* serializer, uint16_t time)
{
	CHECK_NOT_NULL(serializer);
	time++;

	*serializer->error = serializer_mock.error_value;
}

void pico_mqtt_serializer_set_message_id( struct pico_mqtt_serializer* serializer, uint16_t message_id)
{
	CHECK_NOT_NULL(serializer);
	message_id++;

	*serializer->error = serializer_mock.error_value;
}

void pico_mqtt_serializer_set_topic( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data* topic)
{
	CHECK_NOT_NULL(serializer);
	topic->length++;

	*serializer->error = serializer_mock.error_value;
}

void pico_mqtt_serializer_set_quality_of_service( struct pico_mqtt_serializer* serializer, uint8_t qos)
{
	CHECK_NOT_NULL(serializer);
	qos++;

	*serializer->error = serializer_mock.error_value;
}


int pico_mqtt_serialize( struct pico_mqtt_serializer* serializer )
{
	CHECK_NOT_NULL(serializer);
	*serializer->error = serializer_mock.error_value;
	return serializer_mock.return_value;
}

int pico_mqtt_deserialize( struct pico_mqtt_serializer* serializer, struct pico_mqtt_data message)
{
	CHECK_NOT_NULL(serializer);
	CHECK_NOT_NULL(message.data);
	message.length++;

	*serializer->error = serializer_mock.error_value;
	return serializer_mock.return_value;
}

struct pico_mqtt_packet* pico_mqtt_serializer_get_packet( struct pico_mqtt_serializer* serializer)
{
	CHECK_NOT_NULL(serializer);

	*serializer->error = serializer_mock.error_value;
	return serializer_mock.packet;
}

/**
* Debug function implementation
**/

void serializer_mock_reset( void )
{
    serializer_mock = SERIALIZER_MOCK_EMPTY;
}

void serializer_mock_set_create_fail( uint8_t result )
{
  serializer_mock.fail_flag = result;
}

void serializer_mock_set_return_data( struct pico_mqtt_data data )
{
	serializer_mock.return_data = data;
}

void serializer_mock_set_length( uint32_t length )
{
	serializer_mock.length = length;
}

void serializer_mock_set_error_value( int error_value )
{
  serializer_mock.error_value = error_value;
}

void serializer_mock_set_return_value( int return_value )
{
  serializer_mock.return_value = return_value;
}

void serializer_mock_set_packet( struct pico_mqtt_packet* packet )
{
	serializer_mock.packet = packet;
}