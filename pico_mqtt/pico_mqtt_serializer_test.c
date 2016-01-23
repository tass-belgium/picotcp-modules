#define _BSD_SOURCE

#include <check.h>
#include <stdlib.h>

#define ERROR -1
#define SUCCES 0

#define DEBUG 3
#define PEDANTIC

/**
* test data types
**/

/**
* test functions prototypes
**/

int compare_arrays(void* a, void* b, uint32_t length);
Suite * functional_list_suite(void);

/**
* file under test
**/

#include "pico_mqtt_serializer.c"

/**
* tests
**/

START_TEST(create_serializer)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );

	ck_assert_msg( serializer != NULL , "Failed to create the serializer with correct input and enough memory.\n");
	ck_assert_msg( serializer->error == &error , "During construction, the error pointer was not set correctly.\n");
	free(serializer);
}
END_TEST

START_TEST(destroy_stream)
{
	int error = 0;
	struct pico_mqtt_serializer serializer = 
	{
		.packet_id = 1,
		.stream = {.data = malloc(1), .length = 1},
		.stream_ownership = 1,
		.stream_index = (void*)1,
		.error = &error
	};	
	
	destroy_serializer_stream( &serializer );

	ck_assert_msg( serializer.stream.data == NULL , "The stream.data should be cleared.\n");
	ck_assert_msg( serializer.stream.length == 0 , "The stream.length should be cleared.\n");
	ck_assert_msg( serializer.stream_index == 0 , "The stream_index should be cleared.\n");
	ck_assert_msg( serializer.stream_ownership == 0, "The stream was owned by the serializer and shouldn't be any more.\n");
}
END_TEST

START_TEST(clear_serializer)
{
	int error = 0;
	struct pico_mqtt_serializer serializer = 
	{
		.packet_id = 1,
		.stream = {.data = (void*)1, .length = 1},
		.stream_index = (void*)1,
		.error = &error
	};	

	pico_mqtt_serializer_total_reset( &serializer );

	ck_assert_msg( serializer.error != NULL , "Clearing the serializer also removed the pointer to the error, this is incorrect.\n");
	ck_assert_msg( serializer.packet_id == 0 , "The packet_id should be cleared.\n");
	ck_assert_msg( serializer.stream.data == NULL , "The stream.data should be cleared.\n");
	ck_assert_msg( serializer.stream.length == 0 , "The stream.length should be cleared.\n");
	ck_assert_msg( serializer.stream_index == 0 , "The stream_index should be cleared.\n");
}
END_TEST

START_TEST(destroy_serializer)
{
	struct pico_mqtt_serializer* serializer = malloc(sizeof(struct pico_mqtt_serializer));
	pico_mqtt_serializer_total_reset( serializer );

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(create_stream)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 2 );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be allocated, not NULL.\n");
	ck_assert_msg( *(uint8_t*)serializer->stream.data == 0x00 , "The stream.data should be 0x00 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data should be 0x00 at index 1.\n");
	ck_assert_msg( serializer->stream.length == 2 , "The stream.length should be 2, as specified by the test.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data , "The stream_index should be at the beginning of the allocated memory.\n");
	ck_assert_msg( serializer->stream_ownership == 1, "Once a stream is created, the serializer owns it until ownership transfer (get).\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(bytes_left_stream)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint32_t bytes_left = 0;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );
	create_serializer_stream( serializer, 2 );

	bytes_left = get_bytes_left_stream(serializer);

	ck_assert_msg( bytes_left == 2 , "The number of bytes left is calculated wrong.\n");

	serializer->stream_index++;
	bytes_left = get_bytes_left_stream(serializer);

	ck_assert_msg( bytes_left == 1 , "The number of bytes left is calculated wrong.\n");

	serializer->stream_index++;
	bytes_left = get_bytes_left_stream(serializer);

	ck_assert_msg( bytes_left == 0 , "The number of bytes left is calculated wrong.\n");

	serializer->stream_index++;
	bytes_left = get_bytes_left_stream(serializer);

	ck_assert_msg( bytes_left == 0 , "The number of bytes left should be zero, even if the difference is negative.\n");
	
	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(add_byte_stream_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 2 );

	add_byte_stream(serializer, 0xC5);

	ck_assert_msg( *(uint8_t*)serializer->stream.data == 0xC5 , "The stream.data should be 0xC5.\n");
	ck_assert_msg( serializer->stream.length == 2 , "The stream.length should be 2.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data+1 , "The stream_index should be at the beginning+1 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data should be 0x00 at index 0.\n");

	add_byte_stream(serializer, 0x5C);

	ck_assert_msg( *(uint8_t*)serializer->stream.data == 0xC5 , "The stream.data should still be 0xC5.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x5C , "The stream.data should be 0x5C.\n");
	ck_assert_msg( serializer->stream.length == 2 , "The stream.length should be 2.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data+2 , "The stream_index should be at the beginning+1 of the allocated memory.\n");
	
	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(get_byte_stream_test)
{
	int error = 0;
	uint8_t byte = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[2] = { 0xC5, 0x5C };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 2 );
	*((uint8_t*) serializer->stream.data) = message_data[0];
	*((uint8_t*) serializer->stream.data + 1) = message_data[1];

	byte = get_byte_stream(serializer);

	ck_assert_msg( byte == 0xC5 , "The data from the stream should be 0xC5.\n");
	ck_assert_msg( serializer->stream.length == 2 , "The stream.length should be 2.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 1 , "The stream_index should be at the beginning + 1 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0xC5 , "The stream.data should be 0xC5 at index 0.\n");

	byte = get_byte_stream(serializer);

	ck_assert_msg( byte == 0x5C , "The data from the stream should be 0x5C.\n");
	ck_assert_msg( serializer->stream.length == 2 , "The stream.length should be 2.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 2 , "The stream_index should be at the beginning + 2 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x5C , "The stream.data should be 0x5C at index 1.\n");
	
	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(add_2_byte_stream_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 2 );

	add_2_byte_stream(serializer, 0xC5A2);

	ck_assert_msg( *(uint8_t*)serializer->stream.data == 0xC5 , "The stream.data should be 0xC5 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0xA2 , "The stream.data should be 0xA2 at index 1.\n");
	ck_assert_msg( serializer->stream.length == 2 , "The stream.length should be 2.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 2 , "The stream_index should be at the beginning + 2 of the allocated memory.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(get_2_byte_stream_test)
{
	int error = 0;
	uint16_t byte = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[4] = { 0xC5, 0x5C, 0x44, 0x78 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 4 );
	*((uint8_t*) serializer->stream.data) = message_data[0];
	*((uint8_t*) serializer->stream.data + 1) = message_data[1];
	*((uint8_t*) serializer->stream.data + 2) = message_data[2];
	*((uint8_t*) serializer->stream.data + 3) = message_data[3];

	byte = get_2_byte_stream(serializer);

	ck_assert_msg( byte == 0xC55C , "The data from the stream should be 0xC55C.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 2 , "The stream_index should be at the beginning + 2 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0xC5 , "The stream.data should be 0xC5 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x5C , "The stream.data should be 0x5C at index 1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x44 , "The stream.data should be 0x44 at index 2.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x78 , "The stream.data should be 0x78 at index 3.\n");

	byte = get_2_byte_stream(serializer);

	ck_assert_msg( byte == 0x4478 , "The data from the stream should be 0x4478.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be at the beginning + 2 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0xC5 , "The stream.data should be 0xC5 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x5C , "The stream.data should be 0x5C at index 1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x44 , "The stream.data should be 0x44 at index 2.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x78 , "The stream.data should be 0x78 at index 3.\n");
	
	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(add_data_stream_test)
{
	int error = 0;
	uint8_t data_sample_1[2] = {0x48, 0x6E};
	uint8_t data_sample_2[3] = {0xFA, 0xA1, 0X07};
	struct pico_mqtt_serializer* serializer = NULL;

	struct pico_mqtt_data sample_1 = {.data = data_sample_1, .length = 2};
	struct pico_mqtt_data sample_2 = {.data = data_sample_2, .length = 3};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 5 );

	add_data_stream(serializer, sample_1);

	ck_assert_msg( *((uint8_t*)serializer->stream.data + 0) == 0x48 , "The stream.data at index 0 should be 0x48.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x6E , "The stream.data at index 1 should be 0x6E.\n");
	ck_assert_msg( serializer->stream.length == 5 , "The stream.length should be 5.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data+2 , "The stream_index should be at the beginning + 2 of the allocated memory.\n");

	add_data_stream(serializer, sample_2);

	ck_assert_msg( *((uint8_t*)serializer->stream.data + 0) == 0x48 , "The stream.data at index 0 should be 0x48.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x6E , "The stream.data at index 1 should be 0x6E.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0xFA , "The stream.data at index 2 should be 0xFA.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0xA1 , "The stream.data at index 3 should be 0xA1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 4) == 0X07 , "The stream.data at index 4 should be 0X07.\n");
	ck_assert_msg( serializer->stream.length == 5 , "The stream.length should be 5.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 5 , "The stream_index should be at the beginning + 5 of the allocated memory.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(get_data_stream_test)
{
	int error = 0;
	struct pico_mqtt_data data = PICO_MQTT_DATA_ZERO;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[4] = { 0xC5, 0x5C, 0x44, 0x78 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 4 );
	*((uint8_t*) serializer->stream.data) = message_data[0];
	*((uint8_t*) serializer->stream.data + 1) = message_data[1];
	*((uint8_t*) serializer->stream.data + 2) = message_data[2];
	*((uint8_t*) serializer->stream.data + 3) = message_data[3];
	serializer->stream_index++;

	data = get_data_stream(serializer);

	ck_assert_msg( data.length == 3 , "The data from the stream should be 3.\n");
	ck_assert_msg( *((uint8_t*)data.data + 0) == 0x5C , "The data.data should be 0x5C at index 0.\n");
	ck_assert_msg( *((uint8_t*)data.data + 1) == 0x44 , "The data.data should be 0x44 at index 1.\n");
	ck_assert_msg( *((uint8_t*)data.data + 2) == 0x78 , "The data.data should be 0x78 at index 2.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be at the beginning + 4 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0xC5 , "The stream.data should be 0xC5 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x5C , "The stream.data should be 0x5C at index 1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x44 , "The stream.data should be 0x44 at index 2.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x78 , "The stream.data should be 0x78 at index 3.\n");

	free(data.data);
	
	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(add_data_and_length_stream_test)
{
	int error = 0;
	uint8_t data_sample_1[2] = {0x48, 0x6E};
	uint8_t data_sample_2[3] = {0xFA, 0xA1, 0X07};
	struct pico_mqtt_serializer* serializer = NULL;

	struct pico_mqtt_data sample_0 = {.data = NULL, .length = 0};
	struct pico_mqtt_data sample_1 = {.data = data_sample_1, .length = 2};
	struct pico_mqtt_data sample_2 = {.data = data_sample_2, .length = 3};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 11 );

	add_data_and_length_stream(serializer, sample_0);

	ck_assert_msg( *((uint8_t*)serializer->stream.data + 0) == 0x00 , "The stream.data at index 0 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data at index 1 should be 0x00.\n");
	ck_assert_msg( serializer->stream.length == 11 , "The stream.length should be 11.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 2 , "The stream_index should be at the beginning + 2 of the allocated memory.\n");

	add_data_and_length_stream(serializer, sample_1);

	ck_assert_msg( *((uint8_t*)serializer->stream.data + 0) == 0x00 , "The stream.data at index 0 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data at index 1 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x00 , "The stream.data at index 2 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x02 , "The stream.data at index 3 should be 0x02.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 4) == 0x48 , "The stream.data at index 4 should be 0x48.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 5) == 0x6E , "The stream.data at index 5 should be 0x6E.\n");
	ck_assert_msg( serializer->stream.length == 11 , "The stream.length should be 11.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 6 , "The stream_index should be at the beginning + 6 of the allocated memory.\n");

	printf("start test\n");
	add_data_and_length_stream(serializer, sample_2);

	ck_assert_msg( *((uint8_t*)serializer->stream.data + 0) == 0x00 , "The stream.data at index 0 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data at index 1 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x00 , "The stream.data at index 2 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x02 , "The stream.data at index 3 should be 0x02.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 4) == 0x48 , "The stream.data at index 4 should be 0x48.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 5) == 0x6E , "The stream.data at index 5 should be 0x6E.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 6) == 0x00 , "The stream.data at index 6 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 7) == 0x03 , "The stream.data at index 7 should be 0x03.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 8) == 0xFA , "The stream.data at index 8 should be 0xFA.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 9) == 0xA1 , "The stream.data at index 9 should be 0xA1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 10) == 0X07 , "The stream.data at index 10 should be 0X07.\n");
	ck_assert_msg( serializer->stream.length == 11 , "The stream.length should be 11.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 11 , "The stream_index should be at the beginning + 11 of the allocated memory.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(get_data_and_length_stream_test)
{
	int error = 0;
	struct pico_mqtt_data data = PICO_MQTT_DATA_ZERO;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[6] = { 0x57, 0x00, 0x02, 0x5C, 0x44, 0x78 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 6 );
	*((uint8_t*) serializer->stream.data) = message_data[0];
	*((uint8_t*) serializer->stream.data + 1) = message_data[1];
	*((uint8_t*) serializer->stream.data + 2) = message_data[2];
	*((uint8_t*) serializer->stream.data + 3) = message_data[3];
	*((uint8_t*) serializer->stream.data + 4) = message_data[4];
	*((uint8_t*) serializer->stream.data + 5) = message_data[5];
	serializer->stream_index++;

	data = get_data_and_length_stream(serializer);

	ck_assert_msg( data.length == 2 , "The data from the stream should be 2 long.\n");
	ck_assert_msg( *((uint8_t*)data.data + 0) == 0x5C , "The data.data should be 0x5C at index 0.\n");
	ck_assert_msg( *((uint8_t*)data.data + 1) == 0x44 , "The data.data should be 0x44 at index 1.\n");
	ck_assert_msg( serializer->stream.length == 6 , "The stream.length should be 6.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 5 , "The stream_index should be at the beginning + 5 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0x57 , "The stream.data should be 0x57 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data should be 0x00 at index 1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x02 , "The stream.data should be 0x02 at index 2.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x5C , "The stream.data should be 0x5C at index 3.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 4) == 0x44 , "The stream.data should be 0x44 at index 4.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 5) == 0x78 , "The stream.data should be 0x78 at index 5.\n");

	free(data.data);
	
	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(add_array_stream_test)
{
	int error = 0;
	uint8_t data_sample_1[2] = {0x48, 0x6E};
	uint8_t data_sample_2[3] = {0xFA, 0xA1, 0X07};
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 5 );

	add_array_stream(serializer, data_sample_1, 2);

	ck_assert_msg( *((uint8_t*)serializer->stream.data + 0) == 0x48 , "The stream.data at index 0 should be 0x48.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x6E , "The stream.data at index 1 should be 0x6E.\n");
	ck_assert_msg( serializer->stream.length == 5 , "The stream.length should be 5.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data+2 , "The stream_index should be at the beginning + 2 of the allocated memory.\n");

	add_array_stream(serializer, data_sample_2, 3);

	ck_assert_msg( *((uint8_t*)serializer->stream.data + 0) == 0x48 , "The stream.data at index 0 should be 0x48.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x6E , "The stream.data at index 1 should be 0x6E.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0xFA , "The stream.data at index 2 should be 0xFA.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0xA1 , "The stream.data at index 3 should be 0xA1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 4) == 0X07 , "The stream.data at index 4 should be 0X07.\n");
	ck_assert_msg( serializer->stream.length == 5 , "The stream.length should be 5.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 5 , "The stream_index should be at the beginning + 5 of the allocated memory.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(skip_length_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[6] = { 0x08, 0x81, 0x82, 0x83, 0x84, 0x85 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	create_serializer_stream( serializer, 6 );
	*((uint8_t*) serializer->stream.data) = message_data[0];
	*((uint8_t*) serializer->stream.data + 1) = message_data[1];
	*((uint8_t*) serializer->stream.data + 2) = message_data[2];
	*((uint8_t*) serializer->stream.data + 3) = message_data[3];
	*((uint8_t*) serializer->stream.data + 4) = message_data[4];
	*((uint8_t*) serializer->stream.data + 5) = message_data[5];

	skip_length_stream( serializer );

	ck_assert_msg( serializer->stream.length == 6 , "The stream.length should be 6.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 1 , "The stream_index should be at the beginning + 1 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0x08 , "The stream.data should be 0x08 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x81 , "The stream.data should be 0x81 at index 1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x82 , "The stream.data should be 0x82 at index 2.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x83 , "The stream.data should be 0x83 at index 3.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 4) == 0x84 , "The stream.data should be 0x84 at index 4.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 5) == 0x85 , "The stream.data should be 0x85 at index 5.\n");

	skip_length_stream( serializer );

	ck_assert_msg( serializer->stream.length == 6 , "The stream.length should be 6.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 6 , "The stream_index should be at the beginning + 6 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0x08 , "The stream.data should be 0x08 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x81 , "The stream.data should be 0x81 at index 1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x82 , "The stream.data should be 0x82 at index 2.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x83 , "The stream.data should be 0x83 at index 3.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 4) == 0x84 , "The stream.data should be 0x84 at index 4.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 5) == 0x85 , "The stream.data should be 0x85 at index 5.\n");
	
	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(serialize_acknowledge_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_puback[4] = {0x40, 0x02, 0x14, 0x58};
	uint8_t reference_message_pubrec[4] = {0x50, 0x02, 0xAF, 0x7D};
	uint8_t reference_message_pubrel[4] = {0x62, 0x02, 0x1D, 0x5D};
	uint8_t reference_message_pubcomp[4] = {0x70, 0x02, 0x9C, 0xC2};
	uint8_t reference_message_unsuback[4] = {0xB0, 0x02, 0x43, 0x8A};

	serializer = pico_mqtt_serializer_create( &error );

	/* puback */
	pico_mqtt_serializer_total_reset( serializer );
	serializer->packet_id = 0x1458;

	serialize_acknowledge( serializer, PUBACK );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_puback, 4), "The generated output does not match the specifications.\n");

	free(serializer->stream.data);
	/* pubrec */
	pico_mqtt_serializer_total_reset( serializer );
	serializer->packet_id = 0xAF7D;

	serialize_acknowledge( serializer, PUBREC );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubrec, 4), "The generated output does not match the specifications.\n");

	free(serializer->stream.data);
	/* pubrel */
	pico_mqtt_serializer_total_reset( serializer );
	serializer->packet_id = 0x1D5D;

	serialize_acknowledge( serializer, PUBREL );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubrel, 4), "The generated output does not match the specifications.\n");

	free(serializer->stream.data);
	/* pubcomp */
	pico_mqtt_serializer_total_reset( serializer );
	serializer->packet_id = 0x9CC2;

	serialize_acknowledge( serializer, PUBCOMP );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubcomp, 4), "The generated output does not match the specifications.\n");

	free(serializer->stream.data);
	/* unsuback */
	pico_mqtt_serializer_total_reset( serializer );
	serializer->packet_id = 0x438A;

	serialize_acknowledge( serializer, UNSUBACK );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_unsuback, 4), "The generated output does not match the specifications.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(serialize_puback)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_puback[4] = {0x40, 0x02, 0x14, 0x58};

	serializer = pico_mqtt_serializer_create( &error );

	/* puback */
	pico_mqtt_serializer_total_reset( serializer );
	serializer->packet_id = 0x1458;

	pico_mqtt_serialize_puback( serializer );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_puback, 4), "The generated output does not match the specifications.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(serialize_pubrec)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_pubrec[4] = {0x50, 0x02, 0xAF, 0x7D};

	serializer = pico_mqtt_serializer_create( &error );

	/* pubrec */
	pico_mqtt_serializer_total_reset( serializer );
	serializer->packet_id = 0xAF7D;

	pico_mqtt_serialize_pubrec( serializer );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubrec, 4), "The generated output does not match the specifications.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(serialize_pubrel)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_pubrel[4] = {0x62, 0x02, 0x1D, 0x5D};

	serializer = pico_mqtt_serializer_create( &error );

	/* pubrel */
	pico_mqtt_serializer_total_reset( serializer );
	serializer->packet_id = 0x1D5D;

	pico_mqtt_serialize_pubrel( serializer );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubrel, 4), "The generated output does not match the specifications.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(serialize_pubcomp)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_pubcomp[4] = {0x70, 0x02, 0x9C, 0xC2};

	serializer = pico_mqtt_serializer_create( &error );

	pico_mqtt_serializer_total_reset( serializer );
	serializer->packet_id = 0x9CC2;

	pico_mqtt_serialize_pubcomp( serializer );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubcomp, 4), "The generated output does not match the specifications.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

START_TEST(serialize_length)
{
	int error = 0;
	struct pico_mqtt_data result = {.data = NULL, .length = 0};
	uint8_t reference_length_0[1] = {0x00};
	uint8_t reference_length_127[1] = {0x7F};
	uint8_t reference_length_128[2] = {0x80, 0x01};
	uint8_t reference_length_16383[2] = {0xFF, 0x7F};
	uint8_t reference_length_16384[3] = {0x80, 0x80, 0x01};
	uint8_t reference_length_2097151[3] = {0xFF, 0xFF, 0x7F};
	uint8_t reference_length_2097152[4] = {0x80, 0x80, 0x80, 0x01};
	uint8_t reference_length_268435455[4] = {0xFF, 0xFF, 0xFF, 0x7F};
	struct pico_mqtt_serializer* serializer = NULL;
	serializer = pico_mqtt_serializer_create( &error );

	result = *pico_mqtt_serialize_length( serializer, 0 );
	ck_assert_msg( error != ERROR , "A memory allocation error has occured.\n");
	ck_assert_msg( compare_arrays(reference_length_0, result.data, 1), "The generated output does not match the specifications.\n");
	ck_assert_msg( result.length == 1, "The generated output is not of the expected length.\n");
	result.data = NULL;

	result = *pico_mqtt_serialize_length( serializer, 127 );
	ck_assert_msg( error != ERROR , "A memory allocation error has occured.\n");
	ck_assert_msg( compare_arrays(reference_length_127, result.data, 1), "The generated output does not match the specifications.\n");
	ck_assert_msg( result.length == 1, "The generated output is not of the expected length.\n");
	result.data = NULL;

	result = *pico_mqtt_serialize_length( serializer, 128 );
	ck_assert_msg( error != ERROR , "A memory allocation error has occured.\n");
	ck_assert_msg( compare_arrays(reference_length_128, result.data, 2), "The generated output does not match the specifications.\n");
	ck_assert_msg( result.length == 2, "The generated output is not of the expected length.\n");
	result.data = NULL;

	result = *pico_mqtt_serialize_length( serializer, 16383 );
	ck_assert_msg( error != ERROR , "A memory allocation error has occured.\n");
	ck_assert_msg( compare_arrays(reference_length_16383, result.data, 2), "The generated output does not match the specifications.\n");
	ck_assert_msg( result.length == 2, "The generated output is not of the expected length.\n");
	result.data = NULL;

	result = *pico_mqtt_serialize_length( serializer, 16384 );
	ck_assert_msg( error != ERROR , "A memory allocation error has occured.\n");
	ck_assert_msg( compare_arrays(reference_length_16384, result.data, 3), "The generated output does not match the specifications.\n");
	ck_assert_msg( result.length == 3, "The generated output is not of the expected length.\n");
	result.data = NULL;

	result = *pico_mqtt_serialize_length( serializer, 2097151 );
	ck_assert_msg( error != ERROR , "A memory allocation error has occured.\n");
	ck_assert_msg( compare_arrays(reference_length_2097151, result.data, 3), "The generated output does not match the specifications.\n");
	ck_assert_msg( result.length == 3, "The generated output is not of the expected length.\n");
	result.data = NULL;

	result = *pico_mqtt_serialize_length( serializer, 2097152 );
	ck_assert_msg( error != ERROR , "A memory allocation error has occured.\n");
	ck_assert_msg( compare_arrays(reference_length_2097152, result.data, 4), "The generated output does not match the specifications.\n");
	ck_assert_msg( result.length == 4, "The generated output is not of the expected length.\n");
	result.data = NULL;

	result = *pico_mqtt_serialize_length( serializer, 268435455 );
	ck_assert_msg( error != ERROR , "A memory allocation error has occured.\n");
	ck_assert_msg( compare_arrays(reference_length_268435455, result.data, 4), "The generated output does not match the specifications.\n");
	ck_assert_msg( result.length == 4, "The generated output is not of the expected length.\n");
	result.data = NULL;
}
END_TEST

START_TEST(deserialize_length)
{
	int error = 0;
	uint32_t length = 0;
	uint8_t reference_length_0[2] = {0x00, 0xFF};
	uint8_t reference_length_127[2] = {0x7F, 0xFF};
	uint8_t reference_length_128[3] = {0x80, 0x01, 0xFF};
	uint8_t reference_length_16383[3] = {0xFF, 0x7F, 0xFF};
	uint8_t reference_length_16384[4] = {0x80, 0x80, 0x01, 0xFF};
	uint8_t reference_length_2097151[5] = {0xFF, 0xFF, 0x7F, 0xFF};
	uint8_t reference_length_2097152[5] = {0x80, 0x80, 0x80, 0x01, 0xFF};
	uint8_t reference_length_268435455[5] = {0xFF, 0xFF, 0xFF, 0x7F, 0xFF};
	uint8_t reference_length_268435456[6] = {0xFF, 0xFF, 0xFF, 0x80, 0xFF, 0x00};
	struct pico_mqtt_serializer* serializer = NULL;
	serializer = pico_mqtt_serializer_create( &error );

	error = pico_mqtt_deserialize_length( serializer, reference_length_0,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 0, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( serializer, reference_length_127,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 127, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( serializer, reference_length_128,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 128, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( serializer, reference_length_16383,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 16383, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( serializer, reference_length_16384,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 16384, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( serializer, reference_length_2097151,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 2097151, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( serializer, reference_length_2097152,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 2097152, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( serializer, reference_length_268435455,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 268435455, "The generated output does not match the specifications.\n");

	PINFO("Expect an error message to be generated --------------------.\n");
	error = pico_mqtt_deserialize_length( serializer, reference_length_268435456,  &length);
	ck_assert_msg( error == ERROR, "An error should be generated.\n");
}
END_TEST

START_TEST(serializer_set_client_id)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	struct pico_mqtt_data test_data_1 = {.data = (void*) 1, .length = 1};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	pico_mqtt_serializer_set_client_id( serializer, &test_data_1);
	ck_assert_msg( serializer->client_id.length == 1, "The length should be 1.\n");
	ck_assert_msg( serializer->client_id.data == (void*) 1, "The data pointer should be 1.\n");
	ck_assert_msg( serializer->client_id_flag == 1, "The flag should be set");
}
END_TEST

START_TEST(serializer_set_username)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	struct pico_mqtt_data test_data_1 = {.data = (void*) 1, .length = 1};	

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	pico_mqtt_serializer_set_username( serializer, &test_data_1);
	ck_assert_msg( serializer->username.length == 1, "The length should be 1.\n");
	ck_assert_msg( serializer->username.data == (void*) 1, "The data pointer should be 1.\n");
	ck_assert_msg( serializer->username_flag == 1, "The flag should be set");
}
END_TEST

START_TEST(serializer_set_password)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	struct pico_mqtt_data test_data_1 = {.data = (void*) 1, .length = 1};	

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	pico_mqtt_serializer_set_password( serializer, &test_data_1);
	ck_assert_msg( serializer->password.length == 1, "The length should be 1.\n");
	ck_assert_msg( serializer->password.data == (void*) 1, "The data pointer should be 1.\n");
	ck_assert_msg( serializer->password_flag == 1, "The flag should be set");
}
END_TEST

START_TEST(serializer_set_will_topic)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	struct pico_mqtt_data test_data_1 = {.data = (void*) 1, .length = 1};	

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	pico_mqtt_serializer_set_will_topic( serializer, &test_data_1);
	ck_assert_msg( serializer->will_topic.length == 1, "The length should be 1.\n");
	ck_assert_msg( serializer->will_topic.data == (void*) 1, "The data pointer should be 1.\n");
	ck_assert_msg( serializer->will_topic_flag == 1, "The flag should be set");
}
END_TEST

START_TEST(serializer_set_will_message)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	struct pico_mqtt_data test_data_1 = {.data = (void*) 1, .length = 1};	

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	pico_mqtt_serializer_set_will_message( serializer, &test_data_1);
	ck_assert_msg( serializer->will_message.length == 1, "The length should be 1.\n");
	ck_assert_msg( serializer->will_message.data == (void*) 1, "The data pointer should be 1.\n");
	ck_assert_msg( serializer->will_message_flag == 1, "The flag should be set");
}
END_TEST

START_TEST(serialize_connect_check)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	#ifndef ALLOW_EMPTY_CLIENT_ID
		PERROR("ALLOW_EMPTY_CLIENT_ID should be defined but is not, see configuration file.\n");
	#endif

	#if ALLOW_EMPTY_CLIENT_ID == 0
	PINFO("Expect an error message to be generated --------------------.\n");
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == ERROR, "An error should be generated.\n");
	#else
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	#endif

	serializer->client_id_flag = 1;
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");

	serializer->password_flag = 1;
	PINFO("Expect an error message to be generated --------------------.\n");
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == ERROR, "An error should be generated.\n");

	serializer->username_flag = 1;
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");

	serializer->will_topic_flag = 1;
	PINFO("Expect an error message to be generated --------------------.\n");
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == ERROR, "An error should be generated.\n");

	serializer->will_message_flag = 1;
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");

	serializer->will_topic_flag = 0;
	PINFO("Expect an error message to be generated --------------------.\n");
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == ERROR, "An error should be generated.\n");
}
END_TEST

START_TEST(serialize_connect_get_flags_test)
{
	int error = 0;
	uint8_t flags = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x02, "Flags don't match the specifications\n");

	serializer->client_id_flag = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");

	serializer->quality_of_service = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");
	serializer->quality_of_service = 0;

	serializer->clean_session = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x02, "Flags don't match the specifications\n");
	serializer->clean_session = 0;

	serializer->password_flag = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");

	serializer->username_flag = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0xC0, "Flags don't match the specifications\n");

	serializer->password_flag = 0;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x80, "Flags don't match the specifications\n");

	serializer->username_flag = 0;
	serializer->will_retain = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");
	serializer->will_retain = 0;

	serializer->will_topic_flag = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");

	serializer->will_message_flag = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x04, "Flags don't match the specifications\n");

	serializer->will_topic_flag = 0;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");
	serializer->will_topic_flag = 1;

	serializer->will_retain = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x24, "Flags don't match the specifications\n");

	serializer->quality_of_service = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x2C, "Flags don't match the specifications\n");

	serializer->quality_of_service = 2;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x34, "Flags don't match the specifications\n");
}
END_TEST

START_TEST(serialize_connect_get_length_test)
{
	int error = 0;
	uint32_t length = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t client_id_data[4] = { 0x54, 0x67, 0x87, 0xA5 };
	uint8_t username_data[4] = { 0xD2, 0x01, 0x1E, 0x24 };
	uint8_t password_data[4] = { 0x4A, 0x11, 0xFD, 0x9A };
	uint8_t will_topic_data[4] = { 0xFD, 0x21, 0x17, 0x76 };
	uint8_t will_message_data[4] = { 0x63, 0x21, 0xAA, 0x6A };
	struct pico_mqtt_data client_id = {.data = client_id_data, .length = 2};
	struct pico_mqtt_data username = {.data = username_data, .length = 3};
	struct pico_mqtt_data password = {.data = password_data, .length = 4};
	struct pico_mqtt_data will_topic = {.data = will_topic_data, .length = 5};
	struct pico_mqtt_data will_message = {.data = will_message_data, .length = 6};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 12, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_set_client_id( serializer, &client_id);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 14, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_set_password( serializer, &password);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 14, "Length doesn't match the specifications\n");
	pico_mqtt_serializer_total_reset( serializer );
	pico_mqtt_serializer_set_client_id( serializer, &client_id);

	pico_mqtt_serializer_set_username( serializer, &username);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 19, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_set_password( serializer, &password);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 25, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_set_will_topic( serializer, &will_topic);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 25, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_set_will_message( serializer, &will_message);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 40, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_total_reset( serializer );
	pico_mqtt_serializer_set_will_message( serializer, &will_message);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 12, "Length doesn't match the specifications\n");
}
END_TEST

START_TEST(serialize_connect)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t client_id_data[2] = { 0x54, 0x67 };
	uint8_t username_data[3] = { 0xD2, 0x01, 0x1E };
	uint8_t password_data[4] = { 0x4A, 0x11, 0xFD, 0x9A };
	uint8_t will_topic_data[5] = { 0xFD, 0x21, 0x17, 0x76, 0x2F };
	uint8_t will_message_data[6] = { 0x63, 0x21, 0xAA, 0x6A, 0xB7, 0x6A };
	struct pico_mqtt_data client_id = {.data = client_id_data, .length = 2};
	struct pico_mqtt_data username = {.data = username_data, .length = 3};
	struct pico_mqtt_data password = {.data = password_data, .length = 4};
	struct pico_mqtt_data will_topic = {.data = will_topic_data, .length = 5};
	struct pico_mqtt_data will_message = {.data = will_message_data, .length = 6};

	uint8_t reference_message_1[16] = { 0x10, 14, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x00, 0x00, 0x00, 0x00, 0x02, 0x54, 0x67 };
	uint8_t reference_message_2[21] = { 0x10, 19, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x80, 0x00, 0x00, 0x00, 0x02, 0x54, 0x67, 0x00, 0x03, 0xD2, 0x01, 0x1E };
	uint8_t reference_message_3[27] = { 0x10, 25, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0xC0, 0x00, 0x00, 0x00, 0x02, 0x54, 0x67, 0x00, 0x03, 0xD2, 0x01, 0x1E, 0x00, 0x04, 0x4A, 0x11, 0xFD, 0x9A };
	uint8_t reference_message_4[42] = { 0x10, 40, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0xC4, 0x00, 0x00, 0x00, 0x02, 0x54, 0x67, 0x00, 0x05, 0xFD, 0x21, 0x17, 0x76, 0x2F, 0x00, 0x06, 0x63, 0x21, 0xAA, 0x6A, 0xB7, 0x6A, 0x00, 0x03, 0xD2, 0x01, 0x1E, 0x00, 0x04, 0x4A, 0x11, 0xFD, 0x9A};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	pico_mqtt_serializer_set_client_id( serializer, &client_id);
	error = pico_mqtt_serialize_connect( serializer );
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_1, serializer->stream.data, 16), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 16, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_username( serializer, &username);
	error = pico_mqtt_serialize_connect( serializer );
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_2, serializer->stream.data, 21), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 21, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_password( serializer, &password);
	error = pico_mqtt_serialize_connect( serializer );
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_3, serializer->stream.data, 27), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 27, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_will_message( serializer, &will_message);
	pico_mqtt_serializer_set_will_topic( serializer, &will_topic);
	error = pico_mqtt_serialize_connect( serializer );
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_4, serializer->stream.data, 42), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 42, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );
}
END_TEST

START_TEST(deserialize_connack_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t test_data_1[4] = {0x20, 0x01, 0x00, 0x00};
	uint8_t test_data_2[4] = {0x20, 0x03, 0x00, 0x00};
	uint8_t test_data_3[4] = {0x20, 0x02, 0x00, 0x00};
	uint8_t test_data_4[4] = {0x20, 0x02, 0x01, 0x00};
	uint8_t test_data_5[4] = {0x20, 0x02, 0x01, 0x01};
	uint8_t test_data_6[4] = {0x20, 0x02, 0x01, 0x06};
	struct pico_mqtt_data test_case_1 = {.data = test_data_1, .length = 3};
	struct pico_mqtt_data test_case_2 = {.data = test_data_1, .length = 5};
	struct pico_mqtt_data test_case_3 = {.data = test_data_1, .length = 4};
	struct pico_mqtt_data test_case_4 = {.data = test_data_2, .length = 4};
	struct pico_mqtt_data test_case_5 = {.data = test_data_3, .length = 4};
	struct pico_mqtt_data test_case_6 = {.data = test_data_4, .length = 4};
	struct pico_mqtt_data test_case_7 = {.data = test_data_5, .length = 4};
	struct pico_mqtt_data test_case_8 = {.data = test_data_6, .length = 4};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_connack(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_connack(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_connack(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_connack(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_connack(serializer);	
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->session_present == 0, "No Session present flag should be set.\n");
	ck_assert_msg( serializer->return_code == 0, "The return code should be 0.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_connack(serializer);	
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->session_present == 1, "A Session present flag should be set.\n");
	ck_assert_msg( serializer->return_code == 0, "The return code should be 0.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_connack(serializer);	
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->session_present == 1, "A Session present flag should be set.\n");
	ck_assert_msg( serializer->return_code == 1, "The return code should be 1.\n");

	serializer->stream = test_case_8;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_connack(serializer);	
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	ck_assert_msg( serializer->session_present == 1, "A Session present flag should be set.\n");
	ck_assert_msg( serializer->return_code == 6, "The return code should be 6.\n");
}
END_TEST

START_TEST(serialize_publish)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t topic_data[5] = { 0xFD, 0x21, 0x17, 0x76, 0x2F };
	uint8_t message_data[6] = { 0x63, 0x21, 0xAA, 0x6A, 0xB7, 0x6A };
	struct pico_mqtt_data topic = {.data = topic_data, .length = 5};
	struct pico_mqtt_data message = {.data = message_data, .length = 6};
	uint8_t reference_message_1[4] = { 0x30, 2, 0x00, 0x00 };
	uint8_t reference_message_2[6] = { 0x32, 4, 0x00, 0x00, 0x00, 0x00};
	uint8_t reference_message_3[6] = { 0x32, 4, 0x00, 0x00, 0xAA, 0x55};
	uint8_t reference_message_4[6] = { 0x34, 4, 0x00, 0x00, 0xAA, 0x55};
	uint8_t reference_message_5[6] = { 0x35, 4, 0x00, 0x00, 0xAA, 0x55};
	uint8_t reference_message_6[6] = { 0x3D, 4, 0x00, 0x00, 0xAA, 0x55};
	uint8_t reference_message_7[11] = { 0x3D, 9, 0x00, 0x05, 0xFD, 0x21, 0x17, 0x76, 0x2F, 0xAA, 0x55};
	uint8_t reference_message_8[17] = { 0x3D, 15, 0x00, 0x05, 0xFD, 0x21, 0x17, 0x76, 0x2F, 0xAA, 0x55, 0x63, 0x21, 0xAA, 0x6A, 0xB7, 0x6A};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	return_code = pico_mqtt_serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_1, serializer->stream.data, 4), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 4, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 1;
	return_code = pico_mqtt_serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_2, serializer->stream.data, 6), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 6, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 1;
	serializer->packet_id = 0xAA55;
	return_code = pico_mqtt_serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_3, serializer->stream.data, 6), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 6, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 2;
	serializer->packet_id = 0xAA55;
	return_code = pico_mqtt_serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_4, serializer->stream.data, 6), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 6, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 2;
	serializer->packet_id = 0xAA55;
	serializer->retain = 1;
	return_code = pico_mqtt_serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_5, serializer->stream.data, 6), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 6, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 2;
	serializer->packet_id = 0xAA55;
	serializer->retain = 1;
	return_code = pico_mqtt_serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_5, serializer->stream.data, 6), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 6, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 2;
	serializer->packet_id = 0xAA55;
	serializer->retain = 1;
	serializer->duplicate = 1;
	return_code = pico_mqtt_serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_6, serializer->stream.data, 6), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 6, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 2;
	serializer->packet_id = 0xAA55;
	serializer->retain = 1;
	serializer->duplicate = 1;
	serializer->topic = topic;
	return_code = pico_mqtt_serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_7, serializer->stream.data, 7), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 11, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 2;
	serializer->packet_id = 0xAA55;
	serializer->retain = 1;
	serializer->duplicate = 1;
	serializer->topic = topic;
	serializer->message = message;
	return_code = pico_mqtt_serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_8, serializer->stream.data, 17), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 17, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );
}
END_TEST

START_TEST(deserialize_puback_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t test_data_1[4] = {0x40, 0x01, 0x00, 0x00};
	uint8_t test_data_2[4] = {0x40, 0x03, 0x00, 0x00};
	uint8_t test_data_3[4] = {0x40, 0x02, 0x00, 0x00};
	uint8_t test_data_4[4] = {0x40, 0x02, 0x12, 0x34};
	uint8_t test_data_5[4] = {0x40, 0x02, 0x43, 0x21};
	struct pico_mqtt_data test_case_1 = {.data = test_data_1, .length = 3};
	struct pico_mqtt_data test_case_2 = {.data = test_data_1, .length = 5};
	struct pico_mqtt_data test_case_3 = {.data = test_data_1, .length = 4};
	struct pico_mqtt_data test_case_4 = {.data = test_data_2, .length = 4};
	struct pico_mqtt_data test_case_5 = {.data = test_data_3, .length = 4};
	struct pico_mqtt_data test_case_6 = {.data = test_data_4, .length = 4};
	struct pico_mqtt_data test_case_7 = {.data = test_data_5, .length = 4};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x0000, "The session id should be 0x0000.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x1234, "The session id should be 0x1234.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x4321, "The session id should be 0x4321.\n");
}
END_TEST

START_TEST(deserialize_pubrec_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t test_data_1[4] = {0x50, 0x01, 0x00, 0x00};
	uint8_t test_data_2[4] = {0x50, 0x03, 0x00, 0x00};
	uint8_t test_data_3[4] = {0x50, 0x02, 0x00, 0x00};
	uint8_t test_data_4[4] = {0x50, 0x02, 0x12, 0x34};
	uint8_t test_data_5[4] = {0x50, 0x02, 0x43, 0x21};
	struct pico_mqtt_data test_case_1 = {.data = test_data_1, .length = 3};
	struct pico_mqtt_data test_case_2 = {.data = test_data_1, .length = 5};
	struct pico_mqtt_data test_case_3 = {.data = test_data_1, .length = 4};
	struct pico_mqtt_data test_case_4 = {.data = test_data_2, .length = 4};
	struct pico_mqtt_data test_case_5 = {.data = test_data_3, .length = 4};
	struct pico_mqtt_data test_case_6 = {.data = test_data_4, .length = 4};
	struct pico_mqtt_data test_case_7 = {.data = test_data_5, .length = 4};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x0000, "The session id should be 0x0000.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x1234, "The session id should be 0x1234.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x4321, "The session id should be 0x4321.\n");
}
END_TEST

START_TEST(deserialize_pubrel_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t test_data_1[4] = {0x62, 0x01, 0x00, 0x00};
	uint8_t test_data_2[4] = {0x62, 0x03, 0x00, 0x00};
	uint8_t test_data_3[4] = {0x62, 0x02, 0x00, 0x00};
	uint8_t test_data_4[4] = {0x62, 0x02, 0x12, 0x34};
	uint8_t test_data_5[4] = {0x62, 0x02, 0x43, 0x21};
	struct pico_mqtt_data test_case_1 = {.data = test_data_1, .length = 3};
	struct pico_mqtt_data test_case_2 = {.data = test_data_1, .length = 5};
	struct pico_mqtt_data test_case_3 = {.data = test_data_1, .length = 4};
	struct pico_mqtt_data test_case_4 = {.data = test_data_2, .length = 4};
	struct pico_mqtt_data test_case_5 = {.data = test_data_3, .length = 4};
	struct pico_mqtt_data test_case_6 = {.data = test_data_4, .length = 4};
	struct pico_mqtt_data test_case_7 = {.data = test_data_5, .length = 4};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x0000, "The session id should be 0x0000.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x1234, "The session id should be 0x1234.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x4321, "The session id should be 0x4321.\n");
}
END_TEST

START_TEST(deserialize_pubcomp_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t test_data_1[4] = {0x70, 0x01, 0x00, 0x00};
	uint8_t test_data_2[4] = {0x70, 0x03, 0x00, 0x00};
	uint8_t test_data_3[4] = {0x70, 0x02, 0x00, 0x00};
	uint8_t test_data_4[4] = {0x70, 0x02, 0x12, 0x34};
	uint8_t test_data_5[4] = {0x70, 0x02, 0x43, 0x21};
	struct pico_mqtt_data test_case_1 = {.data = test_data_1, .length = 3};
	struct pico_mqtt_data test_case_2 = {.data = test_data_1, .length = 5};
	struct pico_mqtt_data test_case_3 = {.data = test_data_1, .length = 4};
	struct pico_mqtt_data test_case_4 = {.data = test_data_2, .length = 4};
	struct pico_mqtt_data test_case_5 = {.data = test_data_3, .length = 4};
	struct pico_mqtt_data test_case_6 = {.data = test_data_4, .length = 4};
	struct pico_mqtt_data test_case_7 = {.data = test_data_5, .length = 4};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x0000, "The session id should be 0x0000.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x1234, "The session id should be 0x1234.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x4321, "The session id should be 0x4321.\n");
}
END_TEST

START_TEST(deserialize_suback_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t test_data_1[5] = {0x90, 0x02, 0x00, 0x00, 0x00};
	uint8_t test_data_2[5] = {0x90, 0x04, 0x00, 0x00, 0x00};
	uint8_t test_data_3[5] = {0x90, 0x03, 0x00, 0x00, 0x00};
	uint8_t test_data_4[5] = {0x90, 0x03, 0x12, 0x34, 0x00};
	uint8_t test_data_5[5] = {0x90, 0x03, 0x43, 0x21, 0x00};
	uint8_t test_data_6[5] = {0x90, 0x03, 0x43, 0x21, 0x01};
	uint8_t test_data_7[5] = {0x90, 0x03, 0x43, 0x21, 0x02};
	uint8_t test_data_8[5] = {0x90, 0x03, 0x43, 0x21, 0x03};
	uint8_t test_data_9[5] = {0x90, 0x03, 0x43, 0x21, 0x80};
	uint8_t test_data_10[5] = {0x90, 0x03, 0x43, 0x21, 0x0F};
	struct pico_mqtt_data test_case_1 = {.data = test_data_1, .length = 4};
	struct pico_mqtt_data test_case_2 = {.data = test_data_1, .length = 6};
	struct pico_mqtt_data test_case_3 = {.data = test_data_1, .length = 5};
	struct pico_mqtt_data test_case_4 = {.data = test_data_2, .length = 5};
	struct pico_mqtt_data test_case_5 = {.data = test_data_3, .length = 5};
	struct pico_mqtt_data test_case_6 = {.data = test_data_4, .length = 5};
	struct pico_mqtt_data test_case_7 = {.data = test_data_5, .length = 5};
	struct pico_mqtt_data test_case_8 = {.data = test_data_6, .length = 5};
	struct pico_mqtt_data test_case_9 = {.data = test_data_7, .length = 5};
	struct pico_mqtt_data test_case_10 = {.data = test_data_8, .length = 5};
	struct pico_mqtt_data test_case_11 = {.data = test_data_9, .length = 5};
	struct pico_mqtt_data test_case_12 = {.data = test_data_10, .length = 5};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x0000, "The session id should be 0x0000.\n");
	ck_assert_msg( serializer->quality_of_service == 0, "The quality_of_service should be 0.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x1234, "The session id should be 0x1234.\n");
	ck_assert_msg( serializer->quality_of_service == 0, "The quality_of_service should be 0.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x4321, "The session id should be 0x4321.\n");
	ck_assert_msg( serializer->quality_of_service == 0, "The quality_of_service should be 0.\n");

	serializer->stream = test_case_8;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x4321, "The session id should be 0x4321.\n");
	ck_assert_msg( serializer->quality_of_service == 1, "The quality_of_service should be 1.\n");

	serializer->stream = test_case_9;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->packet_id == 0x4321, "The session id should be 0x4321.\n");
	ck_assert_msg( serializer->quality_of_service == 2, "The quality_of_service should be 2.\n");

	serializer->stream = test_case_10;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_11;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_12;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
}
END_TEST

START_TEST(deserialize_publish_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_1[4] = { 0x30, 2, 0x00, 0x00 };
	uint8_t reference_message_2[5] = { 0x30, 3, 0x00, 0x01, 0x92};
	uint8_t reference_message_3[6] = { 0x30, 4, 0x00, 0x01, 0x92, 0x27};
	uint8_t reference_message_4[8] = { 0x32, 4, 0x00, 0x01, 0x92, 0xAA, 0x55, 0x27};
	uint8_t reference_message_5[8] = { 0x33, 4, 0x00, 0x01, 0x92, 0xAA, 0x55, 0x27};
	uint8_t reference_message_6[8] = { 0x3D, 4, 0x00, 0x01, 0x92, 0xAA, 0x55, 0x27};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_total_reset( serializer );

#if ALLOW_EMPTY_TOPIC == 1
	serializer->stream.data = reference_message_1;
	serializer->stream.length = 4;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->topic.length == 0, "The topic should be 0-length.\n");
	ck_assert_msg( serializer->topic.data == NULL, "The topic data pointer should be NULL.\n");
	ck_assert_msg( serializer->quality_of_service == 0, "The Quality of Service should be 0.\n");
	ck_assert_msg( serializer->duplicate == 0, "No duplicate flag should be set.\n");
	ck_assert_msg( serializer->retain == 0, "The retain flag should not be set.\n");
	ck_assert_msg( serializer->packet_id == 0, "Packet Id should be 0.\n");
	ck_assert_msg( serializer->message.length == 0, "The length of the message should be 0.\n");
	ck_assert_msg( serializer->message.data == NULL, "The message data should be NULL.\n");
	pico_mqtt_serializer_clear( serializer );
#else
	serializer->stream.data = reference_message_1;
	serializer->stream.length = 4;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_publish(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");


	serializer->stream.data = reference_message_2;
	serializer->stream.length = 5;
	serializer->stream_index = serializer->stream.data;
	PINFO("Expect an error message to be generated --------------------.\n");
	return_code = deserialize_publish(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
#endif

	serializer->stream.data = reference_message_3;
	serializer->stream.length = 6;
	serializer->stream_index = serializer->stream.data;

	return_code = deserialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->topic.length == 1, "The topic should be length 1.\n");
	ck_assert_msg( serializer->topic.data != NULL, "The topic data pointer should not be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->topic.data) == 0x92, "The topic data should be 0x92.\n");
	ck_assert_msg( serializer->quality_of_service == 0, "The Quality of Service should be 0.\n");
	ck_assert_msg( serializer->duplicate == 0, "No duplicate flag should be set.\n");
	ck_assert_msg( serializer->retain == 0, "The retain flag should not be set.\n");
	ck_assert_msg( serializer->packet_id == 0, "Packet Id should be 0.\n");
	ck_assert_msg( serializer->message.length == 1, "The length of the message should be 1.\n");
	ck_assert_msg( serializer->message.data != NULL, "The message data shouldn't be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->message.data) == 0x27, "The message data should be 0x27.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_4;
	serializer->stream.length = 8;
	serializer->stream_index = serializer->stream.data;

	return_code = deserialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->topic.length == 1, "The topic should be length 1.\n");
	ck_assert_msg( serializer->topic.data != NULL, "The topic data pointer should not be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->topic.data) == 0x92, "The topic data should be 0x92.\n");
	ck_assert_msg( serializer->quality_of_service == 1, "The Quality of Service should be 1.\n");
	ck_assert_msg( serializer->duplicate == 0, "No duplicate flag should be set.\n");
	ck_assert_msg( serializer->retain == 0, "The retain flag should not be set.\n");
	ck_assert_msg( serializer->packet_id == 0xAA55, "Packet Id should be 0xAA55.\n");
	ck_assert_msg( serializer->message.length == 1, "The length of the message should be 1.\n");
	ck_assert_msg( serializer->message.data != NULL, "The message data shouldn't be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->message.data) == 0x27, "The message data should be 0x27.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_5;
	serializer->stream.length = 8;
	serializer->stream_index = serializer->stream.data;

	return_code = deserialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->topic.length == 1, "The topic should be length 1.\n");
	ck_assert_msg( serializer->topic.data != NULL, "The topic data pointer should not be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->topic.data) == 0x92, "The topic data should be 0x92.\n");
	ck_assert_msg( serializer->quality_of_service == 1, "The Quality of Service should be 1.\n");
	ck_assert_msg( serializer->duplicate == 0, "No duplicate flag should be set.\n");
	ck_assert_msg( serializer->retain == 1, "The retain flag should be set.\n");
	ck_assert_msg( serializer->packet_id == 0xAA55, "Packet Id should be 0xAA55.\n");
	ck_assert_msg( serializer->message.length == 1, "The length of the message should be 1.\n");
	ck_assert_msg( serializer->message.data != NULL, "The message data shouldn't be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->message.data) == 0x27, "The message data should be 0x27.\n");
	pico_mqtt_serializer_clear( serializer );


	serializer->stream.data = reference_message_6;
	serializer->stream.length = 8;
	serializer->stream_index = serializer->stream.data;

	return_code = deserialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->topic.length == 1, "The topic should be length 1.\n");
	ck_assert_msg( serializer->topic.data != NULL, "The topic data pointer should not be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->topic.data) == 0x92, "The topic data should be 0x92.\n");
	ck_assert_msg( serializer->quality_of_service == 2, "The Quality of Service should be 2.\n");
	ck_assert_msg( serializer->duplicate == 1, "The duplicate flag should be set.\n");
	ck_assert_msg( serializer->retain == 1, "The retain flag should be set.\n");
	ck_assert_msg( serializer->packet_id == 0xAA55, "Packet Id should be 0xAA55.\n");
	ck_assert_msg( serializer->message.length == 1, "The length of the message should be 1.\n");
	ck_assert_msg( serializer->message.data != NULL, "The message data shouldn't be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->message.data) == 0x27, "The message data should be 0x27.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_destroy( serializer );
}
END_TEST

Suite * functional_list_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("MQTT serializer");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, create_serializer);
	tcase_add_test(test_case_core, clear_serializer);
	tcase_add_test(test_case_core, destroy_stream);
	tcase_add_test(test_case_core, destroy_serializer);
	tcase_add_test(test_case_core, create_stream);
	tcase_add_test(test_case_core, bytes_left_stream);
	tcase_add_test(test_case_core, add_byte_stream_test);
	tcase_add_test(test_case_core, get_byte_stream_test);
	tcase_add_test(test_case_core, add_2_byte_stream_test);
	tcase_add_test(test_case_core, get_2_byte_stream_test);
	tcase_add_test(test_case_core, add_data_stream_test);
	tcase_add_test(test_case_core, get_data_stream_test);
	tcase_add_test(test_case_core, add_array_stream_test);
	tcase_add_test(test_case_core, add_data_and_length_stream_test);
	tcase_add_test(test_case_core, get_data_and_length_stream_test);
	tcase_add_test(test_case_core, skip_length_test);
	tcase_add_test(test_case_core, serialize_acknowledge_test);
	tcase_add_test(test_case_core, serialize_puback);
	tcase_add_test(test_case_core, serialize_pubrec);
	tcase_add_test(test_case_core, serialize_pubrel);
	tcase_add_test(test_case_core, serialize_pubcomp);
	tcase_add_test(test_case_core, serialize_length);
	tcase_add_test(test_case_core, deserialize_length);
	tcase_add_test(test_case_core, serializer_set_client_id);
	tcase_add_test(test_case_core, serializer_set_username);
	tcase_add_test(test_case_core, serializer_set_password);
	tcase_add_test(test_case_core, serializer_set_will_message);
	tcase_add_test(test_case_core, serializer_set_will_topic);
	tcase_add_test(test_case_core, serialize_connect_check);
	tcase_add_test(test_case_core, serialize_connect_get_flags_test);
	tcase_add_test(test_case_core, serialize_connect_get_length_test);
	tcase_add_test(test_case_core, serialize_connect);
	tcase_add_test(test_case_core, deserialize_connack_test);
	tcase_add_test(test_case_core, serialize_publish);
	tcase_add_test(test_case_core, deserialize_puback_test);
	tcase_add_test(test_case_core, deserialize_pubrec_test);
	tcase_add_test(test_case_core, deserialize_pubrel_test);
	tcase_add_test(test_case_core, deserialize_pubcomp_test);
	tcase_add_test(test_case_core, deserialize_suback_test);
	tcase_add_test(test_case_core, deserialize_publish_test);

	suite_add_tcase(test_suite, test_case_core);

	return test_suite;
}

 int main(void)
 {
	int number_failed;
	Suite *test_suite;
	SRunner *suite_runner;

	test_suite = functional_list_suite();
	suite_runner = srunner_create(test_suite);

	srunner_run_all(suite_runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(suite_runner);
	srunner_free(suite_runner);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }

/**
* test functions implementation
**/


int compare_arrays(void* a, void* b, uint32_t length)
{
	uint32_t index = 0;
	for (index = 0; index < length; ++index)
	{
		/*printf("--- %02X - %02X\n", *((uint8_t*)a + index), *((uint8_t*)b + index));*/
		if(*((uint8_t*)a + index) != *((uint8_t*)b + index))
			return 0;
	}

	return 1;
}