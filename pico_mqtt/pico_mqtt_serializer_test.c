#define _BSD_SOURCE

#include <check.h>
#include <stdlib.h>

#define DEBUG 3
#define DISABLE_TRACE

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

START(compare_arrays_test)
{
	uint8_t a1[5] = {1, 2, 3, 4, 5};
	uint8_t a2[5] = {1, 2, 4, 4, 5};
	ck_assert_msg(compare_arrays(a1, a1, 5), "The arrays should match.\n");
	ck_assert_msg(compare_arrays(a2, a2, 5), "The arrays should match.\n");
	ck_assert_msg(!compare_arrays(a1, a2, 5), "The arrays should not match.\n");
	ck_assert_msg(!compare_arrays(a2, a1, 5), "The arrays should not match.\n");

}
END

START(debug_malloc_test)
{
	void* result = NULL;

	MALLOC_SUCCEED();
	result = MALLOC(1);
	ck_assert_msg(result != NULL, "Malloc should not have failed\n");
	FREE(result);

	MALLOC_FAIL();
	result = MALLOC(1);
	ck_assert_msg(result == NULL, "Malloc should have failed\n");

	result = MALLOC(1);
	ck_assert_msg(result == NULL, "Malloc should have failed\n");

	MALLOC_FAIL_ONCE();
	result = MALLOC(1);
	ck_assert_msg(result == NULL, "Malloc should have failed\n");

	result = MALLOC(1);
	ck_assert_msg(result != NULL, "Malloc not should have failed\n");
	FREE(result);

}
END

START(create_serializer)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );

	ck_assert_msg( serializer != NULL , "Failed to create the serializer with correct input and enough memory.\n");
	ck_assert_msg( serializer->error == &error , "During construction, the error pointer was not set correctly.\n");
	FREE(serializer);

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	serializer = pico_mqtt_serializer_create( &error );
	ck_assert_msg( serializer == NULL , "Should return NULL due to malloc error.\n");

}
END

START(destroy_stream)
{
	int error = 0;
	struct pico_mqtt_serializer serializer =
	{
		.message_id = 1,
		.stream = {.data = MALLOC(1), .length = 1},
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
END

START(clear_serializer)
{
	int error = 0;
	struct pico_mqtt_serializer serializer =
	{
		.message_id = 1,
		.stream = {.data = (void*)1, .length = 1},
		.stream_index = (void*)1,
		.error = &error
	};

	pico_mqtt_serializer_clear( &serializer );

	ck_assert_msg( serializer.error != NULL , "Clearing the serializer also removed the pointer to the error, this is incorrect.\n");
	ck_assert_msg( serializer.message_id == 0 , "The message_id should be cleared.\n");
	ck_assert_msg( serializer.stream.data == NULL , "The stream.data should be cleared.\n");
	ck_assert_msg( serializer.stream.length == 0 , "The stream.length should be cleared.\n");
	ck_assert_msg( serializer.stream_index == 0 , "The stream_index should be cleared.\n");

}
END

START(destroy_serializer)
{
	struct pico_mqtt_serializer* serializer = MALLOC(sizeof(struct pico_mqtt_serializer));
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_destroy( serializer );
}
END

START(create_stream_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 2 );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be allocated, not NULL.\n");
	ck_assert_msg( *(uint8_t*)serializer->stream.data == 0x00 , "The stream.data should be 0x00 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data should be 0x00 at index 1.\n");
	ck_assert_msg( serializer->stream.length == 2 , "The stream.length should be 2, as specified by the test.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data , "The stream_index should be at the beginning of the allocated memory.\n");
	ck_assert_msg( serializer->stream_ownership == 1, "Once a stream is created, the serializer owns it until ownership transfer (get).\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(bytes_left_stream)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint32_t bytes_left = 0;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );
	create_stream( serializer, 2 );

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
END

START(add_byte_stream_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 2 );

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
END

START(get_byte_stream_test)
{
	int error = 0;
	uint8_t byte = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[2] = { 0xC5, 0x5C };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 2 );
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
END

START(add_2_byte_stream_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 2 );

	add_2_byte_stream(serializer, 0xC5A2);

	ck_assert_msg( *(uint8_t*)serializer->stream.data == 0xC5 , "The stream.data should be 0xC5 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0xA2 , "The stream.data should be 0xA2 at index 1.\n");
	ck_assert_msg( serializer->stream.length == 2 , "The stream.length should be 2.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 2 , "The stream_index should be at the beginning + 2 of the allocated memory.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(get_2_byte_stream_test)
{
	int error = 0;
	uint16_t byte = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[4] = { 0xC5, 0x5C, 0x44, 0x78 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 4 );
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
END

START(add_data_stream_test)
{
	int error = 0;
	uint8_t data_sample_1[2] = {0x48, 0x6E};
	uint8_t data_sample_2[3] = {0xFA, 0xA1, 0X07};
	struct pico_mqtt_serializer* serializer = NULL;

	struct pico_mqtt_data sample_1 = {.data = data_sample_1, .length = 2};
	struct pico_mqtt_data sample_2 = {.data = data_sample_2, .length = 3};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 5 );

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
END

START(get_data_stream_test)
{
	int error = 0;
	struct pico_mqtt_data* data = NULL;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[4] = { 0xC5, 0x5C, 0x44, 0x78 };

	serializer = pico_mqtt_serializer_create( &error );

	create_stream( serializer, 4 );
	add_data_stream(serializer, (struct pico_mqtt_data){.length = 4, .data = &message_data});
	serializer->stream_index = serializer->stream.data;
	get_byte_stream( serializer );

	data = get_data_stream(serializer);

	ck_assert_msg( data->length == 3 , "The data from the stream should be 3.\n");
	ck_assert_msg( *((uint8_t*)data->data + 0) == 0x5C , "The data.data should be 0x5C at index 0.\n");
	ck_assert_msg( *((uint8_t*)data->data + 1) == 0x44 , "The data.data should be 0x44 at index 1.\n");
	ck_assert_msg( *((uint8_t*)data->data + 2) == 0x78 , "The data.data should be 0x78 at index 2.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be at the beginning + 4 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0xC5 , "The stream.data should be 0xC5 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x5C , "The stream.data should be 0x5C at index 1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x44 , "The stream.data should be 0x44 at index 2.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x78 , "The stream.data should be 0x78 at index 3.\n");
	FREE(data->data);
	FREE(data);
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 4 );
	add_data_stream(serializer, (struct pico_mqtt_data){.length = 4, .data = &message_data});
	serializer->stream_index = serializer->stream.data;
	get_byte_stream( serializer );

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	data = get_data_stream(serializer);
	ck_assert_msg( data == NULL , "The serializer should return NULL.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(add_data_and_length_stream_test)
{
	int error = 0;
	uint8_t data_sample_1[2] = {0x48, 0x6E};
	uint8_t data_sample_2[3] = {0xFA, 0xA1, 0X07};
	struct pico_mqtt_serializer* serializer = NULL;

	struct pico_mqtt_data sample_0 = {.data = NULL, .length = 0};
	struct pico_mqtt_data sample_1 = {.data = data_sample_1, .length = 2};
	struct pico_mqtt_data sample_2 = {.data = data_sample_2, .length = 3};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 11 );

	add_data_and_length_stream(serializer, &sample_0);

	ck_assert_msg( *((uint8_t*)serializer->stream.data + 0) == 0x00 , "The stream.data at index 0 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data at index 1 should be 0x00.\n");
	ck_assert_msg( serializer->stream.length == 11 , "The stream.length should be 11.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 2 , "The stream_index should be at the beginning + 2 of the allocated memory.\n");

	add_data_and_length_stream(serializer, &sample_1);

	ck_assert_msg( *((uint8_t*)serializer->stream.data + 0) == 0x00 , "The stream.data at index 0 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data at index 1 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x00 , "The stream.data at index 2 should be 0x00.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x02 , "The stream.data at index 3 should be 0x02.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 4) == 0x48 , "The stream.data at index 4 should be 0x48.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 5) == 0x6E , "The stream.data at index 5 should be 0x6E.\n");
	ck_assert_msg( serializer->stream.length == 11 , "The stream.length should be 11.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 6 , "The stream_index should be at the beginning + 6 of the allocated memory.\n");

	add_data_and_length_stream(serializer, &sample_2);

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
END

START(get_data_and_length_stream_test)
{
	int error = 0;
	struct pico_mqtt_data* data = NULL;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[6] = { 0x57, 0x00, 0x02, 0x5C, 0x44, 0x78 };

	serializer = pico_mqtt_serializer_create( &error );

	create_stream( serializer, 6 );
	*((uint8_t*) serializer->stream.data) = message_data[0];
	*((uint8_t*) serializer->stream.data + 1) = message_data[1];
	*((uint8_t*) serializer->stream.data + 2) = message_data[2];
	*((uint8_t*) serializer->stream.data + 3) = message_data[3];
	*((uint8_t*) serializer->stream.data + 4) = message_data[4];
	*((uint8_t*) serializer->stream.data + 5) = message_data[5];
	serializer->stream_index++;

	data = get_data_and_length_stream(serializer);
	ck_assert_msg( data->length == 2 , "The data from the stream should be 2 long.\n");
	ck_assert_msg( *((uint8_t*)data->data + 0) == 0x5C , "The data.data should be 0x5C at index 0.\n");
	ck_assert_msg( *((uint8_t*)data->data + 1) == 0x44 , "The data.data should be 0x44 at index 1.\n");
	ck_assert_msg( serializer->stream.length == 6 , "The stream.length should be 6.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 5 , "The stream_index should be at the beginning + 5 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0x57 , "The stream.data should be 0x57 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data should be 0x00 at index 1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x02 , "The stream.data should be 0x02 at index 2.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x5C , "The stream.data should be 0x5C at index 3.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 4) == 0x44 , "The stream.data should be 0x44 at index 4.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 5) == 0x78 , "The stream.data should be 0x78 at index 5.\n");
	FREE(data->data);
	FREE(data);
	pico_mqtt_serializer_clear(serializer);

	create_stream( serializer, 4 );
	*((uint8_t*) serializer->stream.data) = message_data[0];
	*((uint8_t*) serializer->stream.data + 1) = message_data[1];
	*((uint8_t*) serializer->stream.data + 2) = message_data[2];
	*((uint8_t*) serializer->stream.data + 3) = message_data[3];
	serializer->stream_index++;

	PERROR_DISABLE_ONCE();
	data = get_data_and_length_stream(serializer);
	ck_assert_msg( data == NULL , "The data from the stream should be empty.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 3 , "The stream_index should be at the beginning + 3 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0x57 , "The stream.data should be 0x57 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data should be 0x00 at index 1.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 2) == 0x02 , "The stream.data should be 0x02 at index 2.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 3) == 0x5C , "The stream.data should be 0x5C at index 3.\n");
	pico_mqtt_serializer_clear(serializer);

	create_stream( serializer, 2 );
	*((uint8_t*) serializer->stream.data) = message_data[0];
	*((uint8_t*) serializer->stream.data + 1) = message_data[1];
	serializer->stream_index++;

	PERROR_DISABLE_ONCE();
	data = get_data_and_length_stream(serializer);
	ck_assert_msg( data == NULL , "The data from the stream should be empty.\n");
	ck_assert_msg( serializer->stream.length == 2 , "The stream.length should be 2.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 1 , "The stream_index should be at the beginning + 1 of the allocated memory.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data) == 0x57 , "The stream.data should be 0x57 at index 0.\n");
	ck_assert_msg( *((uint8_t*)serializer->stream.data + 1) == 0x00 , "The stream.data should be 0x00 at index 1.\n");
	pico_mqtt_serializer_clear(serializer);

	create_stream( serializer, 6 );
	*((uint8_t*) serializer->stream.data) = message_data[0];
	*((uint8_t*) serializer->stream.data + 1) = message_data[1];
	*((uint8_t*) serializer->stream.data + 2) = message_data[2];
	*((uint8_t*) serializer->stream.data + 3) = message_data[3];
	*((uint8_t*) serializer->stream.data + 4) = message_data[4];
	*((uint8_t*) serializer->stream.data + 5) = message_data[5];
	serializer->stream_index++;

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	data = get_data_and_length_stream(serializer);
	ck_assert_msg( data == NULL , "The data from the stream should be empty.\n");;
	pico_mqtt_serializer_clear(serializer);

	pico_mqtt_serializer_destroy( serializer );
}
END

START(add_array_stream_test)
{
	int error = 0;
	uint8_t data_sample_1[2] = {0x48, 0x6E};
	uint8_t data_sample_2[3] = {0xFA, 0xA1, 0X07};
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 5 );

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
END

START(skip_length_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t message_data[6] = { 0x08, 0x81, 0x82, 0x83, 0x84, 0x85 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	create_stream( serializer, 6 );
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
END

START(serialize_acknowledge_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_puback[4] = {0x40, 0x02, 0x14, 0x58};
	uint8_t reference_message_pubrec[4] = {0x50, 0x02, 0xAF, 0x7D};
	uint8_t reference_message_pubrel[4] = {0x62, 0x02, 0x1D, 0x5D};
	uint8_t reference_message_pubcomp[4] = {0x70, 0x02, 0x9C, 0xC2};
	uint8_t reference_message_unsuback[4] = {0xB0, 0x02, 0x43, 0x8A};

	serializer = pico_mqtt_serializer_create( &error );

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	serialize_acknowledge( serializer, PUBACK );

	ck_assert_msg( serializer->stream.data == NULL , "The stream.data should not be set.\n");
	ck_assert_msg( serializer->stream.length == 0 , "The stream.length should be 0 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data , "The stream_index should be beginning.\n");

	// puback 
	pico_mqtt_serializer_clear( serializer );
	serializer->message_id = 0x1458;

	serialize_acknowledge( serializer, PUBACK );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_puback, 4), "The generated output does not match the specifications.\n");

	// pubrec
	pico_mqtt_serializer_clear( serializer );
	serializer->message_id = 0xAF7D;

	serialize_acknowledge( serializer, PUBREC );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubrec, 4), "The generated output does not match the specifications.\n");

	// pubrel 
	pico_mqtt_serializer_clear( serializer );
	serializer->message_id = 0x1D5D;

	serialize_acknowledge( serializer, PUBREL );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubrel, 4), "The generated output does not match the specifications.\n");

	// pubcomp 
	pico_mqtt_serializer_clear( serializer );
	serializer->message_id = 0x9CC2;

	serialize_acknowledge( serializer, PUBCOMP );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubcomp, 4), "The generated output does not match the specifications.\n");

	// unsuback 
	pico_mqtt_serializer_clear( serializer );
	serializer->message_id = 0x438A;

	serialize_acknowledge( serializer, UNSUBACK );

	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_unsuback, 4), "The generated output does not match the specifications.\n");


	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_puback_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_puback[4] = {0x40, 0x02, 0x14, 0x58};

	serializer = pico_mqtt_serializer_create( &error );

	// puback
	pico_mqtt_serializer_clear( serializer );
	serializer->message_id = 0x1458;

	return_code = serialize_puback( serializer );

	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_puback, 4), "The generated output does not match the specifications.\n");


	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_pubrec_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_pubrec[4] = {0x50, 0x02, 0xAF, 0x7D};

	serializer = pico_mqtt_serializer_create( &error );

	pico_mqtt_serializer_clear( serializer );
	serializer->message_id = 0xAF7D;

	return_code = serialize_pubrec( serializer );

	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubrec, 4), "The generated output does not match the specifications.\n");


	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_pubrel_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_pubrel[4] = {0x62, 0x02, 0x1D, 0x5D};

	serializer = pico_mqtt_serializer_create( &error );

	pico_mqtt_serializer_clear( serializer );
	serializer->message_id = 0x1D5D;

	return_code = serialize_pubrel( serializer );

	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubrel, 4), "The generated output does not match the specifications.\n");


	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_pubcomp_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t reference_message_pubcomp[4] = {0x70, 0x02, 0x9C, 0xC2};

	serializer = pico_mqtt_serializer_create( &error );

	pico_mqtt_serializer_clear( serializer );
	serializer->message_id = 0x9CC2;

	return_code = serialize_pubcomp( serializer );

	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->stream.data != NULL , "The stream.data should be set.\n");
	ck_assert_msg( serializer->stream.length == 4 , "The stream.length should be 4 bytes long.\n");
	ck_assert_msg( serializer->stream_index == serializer->stream.data + 4 , "The stream_index should be beginning + 4.\n");
	ck_assert_msg( compare_arrays(serializer->stream.data, reference_message_pubcomp, 4), "The generated output does not match the specifications.\n");


	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_length)
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

	pico_mqtt_serializer_destroy(serializer);
}
END

START(deserialize_length)
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

	error = pico_mqtt_deserialize_length( &error, reference_length_0,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 0, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( &error, reference_length_127,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 127, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( &error, reference_length_128,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 128, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( &error, reference_length_16383,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 16383, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( &error, reference_length_16384,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 16384, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( &error, reference_length_2097151,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 2097151, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( &error, reference_length_2097152,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 2097152, "The generated output does not match the specifications.\n");

	error = pico_mqtt_deserialize_length( &error, reference_length_268435455,  &length);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( length == 268435455, "The generated output does not match the specifications.\n");

	PERROR_DISABLE_ONCE();
	error = pico_mqtt_deserialize_length( &error, reference_length_268435456,  &length);
	ck_assert_msg( error == ERROR, "An error should be generated.\n");

}
END

START(pico_mqtt_serializer_set_message_type_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	serializer = pico_mqtt_serializer_create( &error );

	pico_mqtt_serializer_set_message_type( serializer, 7);
	ck_assert_msg( serializer->message_type == 7, "The message type should be 7.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(serializer_set_client_id)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	struct pico_mqtt_data test_data_1 = {.data = (void*) 1, .length = 1};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

#if ALLOW_EMPTY_CLIENT_ID == 1

	pico_mqtt_serializer_set_client_id( serializer, NULL);
	ck_assert_msg( serializer->client_id == NULL, "The data pointer should be NULL.\n");
	pico_mqtt_serializer_clear( serializer );

#endif

	pico_mqtt_serializer_set_client_id( serializer, &test_data_1);
	ck_assert_msg( serializer->client_id->length == 1, "The length should be 1.\n");
	ck_assert_msg( serializer->client_id->data == (void*) 1, "The data pointer should be 1.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(serializer_set_username)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	struct pico_mqtt_data test_data_1 = {.data = (void*) 1, .length = 1};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

#if ALLOW_EMPTY_USERNAME == 1

	pico_mqtt_serializer_set_username( serializer, NULL);
	ck_assert_msg( serializer->username == NULL, "The data pointer should be NULL.\n");
	pico_mqtt_serializer_clear( serializer );

#endif

	pico_mqtt_serializer_set_username( serializer, &test_data_1);
	ck_assert_msg( serializer->username->length == 1, "The length should be 1.\n");
	ck_assert_msg( serializer->username->data == (void*) 1, "The data pointer should be 1.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(serializer_set_password)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	struct pico_mqtt_data test_data_1 = {.data = (void*) 1, .length = 1};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_password( serializer, NULL);
	ck_assert_msg( serializer->password == NULL, "The data pointer should be NULL.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_password( serializer, &test_data_1);
	ck_assert_msg( serializer->password->length == 1, "The length should be 1.\n");
	ck_assert_msg( serializer->password->data == (void*) 1, "The data pointer should be 1.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(serializer_set_message_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	struct pico_mqtt_data topic = {.length=1, .data = (void*)1};
	struct pico_mqtt_data message = {.length=1, .data = (void*)1};
	struct pico_mqtt_message test_data_1 = {.retain = 1, .topic = &topic, .data = &message};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_message( serializer, &test_data_1);
	ck_assert_msg( serializer->retain == 1, "The retain flag should be set\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_destroy(serializer);
}
END

START(pico_mqtt_serializer_set_keep_alive_time_test)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	serializer = pico_mqtt_serializer_create( &error );

	pico_mqtt_serializer_set_keep_alive_time( serializer, 7);
	ck_assert_msg( serializer->keep_alive == 7, "The keep alive time should be 7.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(serialize_connect_check)
{
	int error = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	#ifndef ALLOW_EMPTY_CLIENT_ID
		PERROR("ALLOW_EMPTY_CLIENT_ID should be defined but is not, see configuration file.\n");
	#endif

	#if ALLOW_EMPTY_CLIENT_ID == 0
	PERROR_DISABLE_ONCE();
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == ERROR, "An error should be generated.\n");
	#else
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	#endif

	serializer->client_id = (struct pico_mqtt_data*) 1;
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");

	serializer->password = (struct pico_mqtt_data*) 1;;
	PERROR_DISABLE_ONCE();
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == ERROR, "An error should be generated.\n");

	serializer->username = (struct pico_mqtt_data*) 1;;
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");

	serializer->topic = (struct pico_mqtt_data*) 1;
	PERROR_DISABLE_ONCE();
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");

	serializer->data = (struct pico_mqtt_data*) 1;
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");

	serializer->topic = NULL;
	PERROR_DISABLE_ONCE();
	error = check_serialize_connect( serializer);
	ck_assert_msg( error == ERROR, "An error should be generated.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(serialize_connect_get_flags_test)
{
	int error = 0;
	uint8_t flags = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x02, "Flags don't match the specifications\n");

	serializer->client_id = (struct pico_mqtt_data*) 1;
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

	serializer->password = (struct pico_mqtt_data*) 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");

	serializer->username = (struct pico_mqtt_data*) 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0xC0, "Flags don't match the specifications\n");

	serializer->password = NULL;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x80, "Flags don't match the specifications\n");

	serializer->username = NULL;
	serializer->retain = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");
	serializer->retain = 0;

	serializer->topic = (struct pico_mqtt_data*) 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");

	serializer->data = (struct pico_mqtt_data*) 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x04, "Flags don't match the specifications\n");

	serializer->topic = NULL;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x00, "Flags don't match the specifications\n");
	serializer->topic = (struct pico_mqtt_data*) 1;

	serializer->retain = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x24, "Flags don't match the specifications\n");

	serializer->quality_of_service = 1;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x2C, "Flags don't match the specifications\n");

	serializer->quality_of_service = 2;
	flags = serialize_connect_get_flags( serializer);
	ck_assert_msg( flags == 0x34, "Flags don't match the specifications\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(serialize_connect_get_length_test)
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
	struct pico_mqtt_message message = {.topic = &will_topic, .data = &will_message};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 12, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_set_client_id( serializer, &client_id);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 14, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_set_password( serializer, &password);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 14, "Length doesn't match the specifications\n");
	pico_mqtt_serializer_clear( serializer );
	pico_mqtt_serializer_set_client_id( serializer, &client_id);

	pico_mqtt_serializer_set_username( serializer, &username);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 19, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_set_password( serializer, &password);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 25, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_set_message( serializer, &message);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 40, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_clear( serializer );
	pico_mqtt_serializer_set_client_id( serializer, &client_id);
	pico_mqtt_serializer_set_message( serializer, &message);
	length = serialize_connect_get_length( serializer);
	ck_assert_msg( length == 29, "Length doesn't match the specifications\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(serialize_connect_test)
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
	struct pico_mqtt_message message = {.topic = &will_topic, .data = &will_message};

	uint8_t reference_message_1[16] = { 0x10, 14, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x00, 0x00, 0x00, 0x00, 0x02, 0x54, 0x67 };
	uint8_t reference_message_2[21] = { 0x10, 19, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x80, 0x00, 0x00, 0x00, 0x02, 0x54, 0x67, 0x00, 0x03, 0xD2, 0x01, 0x1E };
	uint8_t reference_message_3[27] = { 0x10, 25, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0xC0, 0x00, 0x00, 0x00, 0x02, 0x54, 0x67, 0x00, 0x03, 0xD2, 0x01, 0x1E, 0x00, 0x04, 0x4A, 0x11, 0xFD, 0x9A };
	uint8_t reference_message_4[42] = { 0x10, 40, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0xC4, 0x00, 0x00, 0x00, 0x02, 0x54, 0x67, 0x00, 0x05, 0xFD, 0x21, 0x17, 0x76, 0x2F, 0x00, 0x06, 0x63, 0x21, 0xAA, 0x6A, 0xB7, 0x6A, 0x00, 0x03, 0xD2, 0x01, 0x1E, 0x00, 0x04, 0x4A, 0x11, 0xFD, 0x9A};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_client_id( serializer, &client_id);
	error = serialize_connect( serializer );
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_1, serializer->stream.data, 16), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 16, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	pico_mqtt_serializer_set_client_id( serializer, &client_id);
	error = serialize_connect( serializer );
	ck_assert_msg( error == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_client_id( serializer, &client_id);
	pico_mqtt_serializer_set_username( serializer, &username);
	error = serialize_connect( serializer );
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_2, serializer->stream.data, 21), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 21, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_client_id( serializer, &client_id);
	pico_mqtt_serializer_set_username( serializer, &username);
	pico_mqtt_serializer_set_password( serializer, &password);
	error = serialize_connect( serializer );
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_3, serializer->stream.data, 27), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 27, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_set_client_id( serializer, &client_id);
	pico_mqtt_serializer_set_username( serializer, &username);
	pico_mqtt_serializer_set_password( serializer, &password);
	pico_mqtt_serializer_set_message( serializer, &message);
	error = serialize_connect( serializer );
	ck_assert_msg( error == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_4, serializer->stream.data, 42), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 42, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer);
	CHECK_ALLOCATIONS(1);

	pico_mqtt_serializer_destroy( serializer );
}
END

START(deserialize_connack_test)
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
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_connack(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_connack(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_connack(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
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
	PERROR_DISABLE_ONCE();
	return_code = deserialize_connack(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	ck_assert_msg( serializer->session_present == 1, "A Session present flag should be set.\n");
	ck_assert_msg( serializer->return_code == 6, "The return code should be 6.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(serialize_publish_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t topic_data[5] = { 0xFD, 0x21, 0x17, 0x76, 0x2F };
	uint8_t message_data[6] = { 0x63, 0x21, 0xAA, 0x6A, 0xB7, 0x6A };
	struct pico_mqtt_data topic = {.data = topic_data, .length = 5};
	struct pico_mqtt_data message = {.data = message_data, .length = 6};
	uint8_t reference_message_8[17] = { 0x3D, 15, 0x00, 0x05, 0xFD, 0x21, 0x17, 0x76, 0x2F, 0xAA, 0x55, 0x63, 0x21, 0xAA, 0x6A, 0xB7, 0x6A};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	PERROR_DISABLE_ONCE();
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );
	MALLOC_SUCCEED();

	serializer->quality_of_service = 1;
	PERROR_DISABLE_ONCE();
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->quality_of_service = 1;
	serializer->message_id = 0xAA55;

	PERROR_DISABLE_ONCE();
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->quality_of_service = 2;
	serializer->message_id = 0xAA55;

	PERROR_DISABLE_ONCE();
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->quality_of_service = 2;
	serializer->message_id = 0xAA55;
	serializer->retain = 1;

	PERROR_DISABLE_ONCE();
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->quality_of_service = 2;
	serializer->message_id = 0xAA55;
	serializer->retain = 1;

	PERROR_DISABLE_ONCE();
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->quality_of_service = 2;
	serializer->message_id = 0xAA55;
	serializer->retain = 1;
	serializer->duplicate = 1;

	PERROR_DISABLE_ONCE();
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->quality_of_service = 2;
	serializer->message_id = 0xAA55;
	serializer->retain = 1;
	serializer->duplicate = 1;
	serializer->topic = &topic;

	PERROR_DISABLE_ONCE();
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	pico_mqtt_serializer_clear(serializer);


	serializer->quality_of_service = 2;
	serializer->message_id = 0xAA55;
	serializer->retain = 1;
	serializer->duplicate = 1;
	serializer->topic = &topic;
	serializer->data = &message;
	return_code = serialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_8, serializer->stream.data, 17), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 17, "The generated output is not of the expected length.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(deserialize_puback_test)
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
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x0000, "The session id should be 0x0000.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x1234, "The session id should be 0x1234.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x4321, "The session id should be 0x4321.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(deserialize_pubrec_test)
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
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x0000, "The session id should be 0x0000.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x1234, "The session id should be 0x1234.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x4321, "The session id should be 0x4321.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(deserialize_pubrel_test)
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
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x0000, "The session id should be 0x0000.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x1234, "The session id should be 0x1234.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x4321, "The session id should be 0x4321.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(deserialize_pubcomp_test)
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
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x0000, "The session id should be 0x0000.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x1234, "The session id should be 0x1234.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x4321, "The session id should be 0x4321.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(deserialize_unsuback_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t test_data_1[4] = {0xB0, 0x01, 0x00, 0x00};
	uint8_t test_data_2[4] = {0xB0, 0x03, 0x00, 0x00};
	uint8_t test_data_3[4] = {0xB0, 0x02, 0x00, 0x00};
	uint8_t test_data_4[4] = {0xB0, 0x02, 0x12, 0x34};
	uint8_t test_data_5[4] = {0xB0, 0x02, 0x43, 0x21};
	struct pico_mqtt_data test_case_1 = {.data = test_data_1, .length = 3};
	struct pico_mqtt_data test_case_2 = {.data = test_data_1, .length = 5};
	struct pico_mqtt_data test_case_3 = {.data = test_data_1, .length = 4};
	struct pico_mqtt_data test_case_4 = {.data = test_data_2, .length = 4};
	struct pico_mqtt_data test_case_5 = {.data = test_data_3, .length = 4};
	struct pico_mqtt_data test_case_6 = {.data = test_data_4, .length = 4};
	struct pico_mqtt_data test_case_7 = {.data = test_data_5, .length = 4};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_acknowledge(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x0000, "The session id should be 0x0000.\n");

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x1234, "The session id should be 0x1234.\n");

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_acknowledge(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x4321, "The session id should be 0x4321.\n");

	pico_mqtt_serializer_destroy(serializer);
}
END

START(deserialize_suback_test)
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
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_1;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_2;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_3;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_4;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_5;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x0000, "The session id should be 0x0000.\n");
	ck_assert_msg( serializer->quality_of_service == 0, "The quality_of_service should be 0.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_6;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x1234, "The session id should be 0x1234.\n");
	ck_assert_msg( serializer->quality_of_service == 0, "The quality_of_service should be 0.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_7;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x4321, "The session id should be 0x4321.\n");
	ck_assert_msg( serializer->quality_of_service == 0, "The quality_of_service should be 0.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_8;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x4321, "The session id should be 0x4321.\n");
	ck_assert_msg( serializer->quality_of_service == 1, "The quality_of_service should be 1.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_9;
	serializer->stream_index = serializer->stream.data;
	return_code = deserialize_suback(serializer);	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_id == 0x4321, "The session id should be 0x4321.\n");
	ck_assert_msg( serializer->quality_of_service == 2, "The quality_of_service should be 2.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_10;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_11;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream = test_case_12;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_suback(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_destroy( serializer );
}
END

START(deserialize_publish_test)
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
	uint8_t reference_message_7[8] = { 0x3F, 4, 0x00, 0x01, 0x92, 0xAA, 0x55, 0x27};

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_1;
	serializer->stream.length = 3;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_publish(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream.data = reference_message_1;
	serializer->stream.length = 4;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_publish(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_2;
	serializer->stream.length = 5;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_publish(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_3;
	serializer->stream.length = 6;
	serializer->stream_index = serializer->stream.data;

	return_code = deserialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->topic->length == 1, "The topic should be length 1.\n");
	ck_assert_msg( serializer->topic->data != NULL, "The topic data pointer should not be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->topic->data) == 0x92, "The topic data should be 0x92.\n");
	ck_assert_msg( serializer->quality_of_service == 0, "The Quality of Service should be 0.\n");
	ck_assert_msg( serializer->duplicate == 0, "No duplicate flag should be set.\n");
	ck_assert_msg( serializer->retain == 0, "The retain flag should not be set.\n");
	ck_assert_msg( serializer->message_id == 0, "Packet Id should be 0.\n");
	ck_assert_msg( serializer->data->length == 1, "The length of the message should be 1.\n");
	ck_assert_msg( serializer->data->data != NULL, "The message data shouldn't be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->data->data) == 0x27, "The message data should be 0x27.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_4;
	serializer->stream.length = 8;
	serializer->stream_index = serializer->stream.data;

	return_code = deserialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->topic->length == 1, "The topic should be length 1.\n");
	ck_assert_msg( serializer->topic->data != NULL, "The topic data pointer should not be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->topic->data) == 0x92, "The topic data should be 0x92.\n");
	ck_assert_msg( serializer->quality_of_service == 1, "The Quality of Service should be 1.\n");
	ck_assert_msg( serializer->duplicate == 0, "No duplicate flag should be set.\n");
	ck_assert_msg( serializer->retain == 0, "The retain flag should not be set.\n");
	ck_assert_msg( serializer->message_id == 0xAA55, "Packet Id should be 0xAA55.\n");
	ck_assert_msg( serializer->data->length == 1, "The length of the message should be 1.\n");
	ck_assert_msg( serializer->data->data != NULL, "The message data shouldn't be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->data->data) == 0x27, "The message data should be 0x27.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_5;
	serializer->stream.length = 8;
	serializer->stream_index = serializer->stream.data;

	return_code = deserialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->topic->length == 1, "The topic should be length 1.\n");
	ck_assert_msg( serializer->topic->data != NULL, "The topic data pointer should not be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->topic->data) == 0x92, "The topic data should be 0x92.\n");
	ck_assert_msg( serializer->quality_of_service == 1, "The Quality of Service should be 1.\n");
	ck_assert_msg( serializer->duplicate == 0, "No duplicate flag should be set.\n");
	ck_assert_msg( serializer->retain == 1, "The retain flag should be set.\n");
	ck_assert_msg( serializer->message_id == 0xAA55, "Packet Id should be 0xAA55.\n");
	ck_assert_msg( serializer->data->length == 1, "The length of the message should be 1.\n");
	ck_assert_msg( serializer->data->data != NULL, "The message data shouldn't be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->data->data) == 0x27, "The message data should be 0x27.\n");
	pico_mqtt_serializer_clear( serializer );


	serializer->stream.data = reference_message_6;
	serializer->stream.length = 8;
	serializer->stream_index = serializer->stream.data;

	return_code = deserialize_publish( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->topic->length == 1, "The topic should be length 1.\n");
	ck_assert_msg( serializer->topic->data != NULL, "The topic data pointer should not be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->topic->data) == 0x92, "The topic data should be 0x92.\n");
	ck_assert_msg( serializer->quality_of_service == 2, "The Quality of Service should be 2.\n");
	ck_assert_msg( serializer->duplicate == 1, "The duplicate flag should be set.\n");
	ck_assert_msg( serializer->retain == 1, "The retain flag should be set.\n");
	ck_assert_msg( serializer->message_id == 0xAA55, "Packet Id should be 0xAA55.\n");
	ck_assert_msg( serializer->data->length == 1, "The length of the message should be 1.\n");
	ck_assert_msg( serializer->data->data != NULL, "The message data shouldn't be NULL.\n");
	ck_assert_msg( *((uint8_t *)serializer->data->data) == 0x27, "The message data should be 0x27.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_7;
	serializer->stream.length = 8;
	serializer->stream_index = serializer->stream.data;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_publish(serializer);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_subscribe_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t topic_data[2] = {0x12, 0x34};
	struct pico_mqtt_data topic = {.data = topic_data, .length = 2};
	uint8_t reference_message_1[9] = { 0x82, 7, 0xAA, 0x55, 0x00, 0x02, 0x12, 0x34, 0x01 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 1;
	serializer->message_id = 0xAA55;
	serializer->topic = &topic;
	return_code = serialize_subscribe( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_1, serializer->stream.data, 9), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 9, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );


	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_unsubscribe_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t topic_data[2] = {0x12, 0x34};
	struct pico_mqtt_data topic = {.data = topic_data, .length = 2};
	uint8_t reference_message_1[9] = { 0xA2, 7, 0xAA, 0x55, 0x00, 0x02, 0x12, 0x34, 0x01 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 1;
	serializer->message_id = 0xAA55;
	serializer->topic = &topic;
	return_code = serialize_unsubscribe( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_1, serializer->stream.data, 9), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 9, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_subscribtion_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	uint8_t topic_data[2] = {0x12, 0x34};
	struct pico_mqtt_data topic = {.data = topic_data, .length = 2};
	uint8_t reference_message_3[9] = { 0x82, 7, 0xAA, 0x55, 0x00, 0x02, 0x12, 0x34, 0x01 };
	uint8_t reference_message_4[9] = { 0xA2, 7, 0xAA, 0x55, 0x00, 0x02, 0x12, 0x34, 0x01 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	PERROR_DISABLE_ONCE();
	return_code = serialize_subscribtion( serializer , 1);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	PERROR_DISABLE_ONCE();
	return_code = serialize_subscribtion( serializer , 0);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 1;
	serializer->message_id = 0xAA55;
	serializer->topic = &topic;
	return_code = serialize_subscribtion( serializer , 1);
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_3, serializer->stream.data, 9), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 9, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear( serializer );

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	serializer->quality_of_service = 1;
	serializer->message_id = 0xAA55;
	serializer->topic = &topic;
	return_code = serialize_subscribtion( serializer , 1);
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");
	pico_mqtt_serializer_clear( serializer );

	serializer->quality_of_service = 1;
	serializer->message_id = 0xAA55;
	serializer->topic = &topic;
	return_code = serialize_subscribtion( serializer , 0);
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_4, serializer->stream.data, 9), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 9, "The generated output is not of the expected length.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_pingreq_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	uint8_t reference_message_1[2] = { 0xC0, 0x00 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	return_code = serialize_pingreq( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_1, serializer->stream.data, 2), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 2, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear(serializer);

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	return_code = serialize_pingreq( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	pico_mqtt_serializer_destroy( serializer );

}
END

START(serialize_pingresp_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	uint8_t reference_message_1[2] = { 0xD0, 0x00 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	return_code = serialize_pingresp( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_1, serializer->stream.data, 2), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 2, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear(serializer);

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	return_code = serialize_pingresp( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(serialize_disconnect_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	uint8_t reference_message_1[2] = { 0xE0, 0x00 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	return_code = serialize_disconnect( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( compare_arrays(reference_message_1, serializer->stream.data, 2), "The generated output does not match the specifications.\n");
	ck_assert_msg( serializer->stream.length == 2, "The generated output is not of the expected length.\n");
	pico_mqtt_serializer_clear(serializer);

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	return_code = serialize_disconnect( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(deserialize_pingreq_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	uint8_t reference_message_1[2] = { 0xC0, 0x00 };
	uint8_t reference_message_2[2] = { 0x10, 0x00 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_1;
	serializer->stream_index = serializer->stream.data;
	serializer->stream.length = 3;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_pingreq( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream.data = reference_message_2;
	serializer->stream_index = serializer->stream.data;
	serializer->stream.length = 2;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_pingreq( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream.data = reference_message_1;
	serializer->stream_index = serializer->stream.data;
	serializer->stream.length = 2;
	return_code = deserialize_pingreq( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_type == 12, "The message type is not set correctly.\n");
	ck_assert_msg( serializer->stream.length == 2, "The generated output is not of the expected length.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(deserialize_pingresp_test)
{
	int error = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;

	uint8_t reference_message_1[2] = { 0xD0, 0x00 };
	uint8_t reference_message_2[2] = { 0x10, 0x00 };

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	serializer->stream.data = reference_message_1;
	serializer->stream_index = serializer->stream.data;
	serializer->stream.length = 3;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_pingresp( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream.data = reference_message_2;
	serializer->stream_index = serializer->stream.data;
	serializer->stream.length = 2;
	PERROR_DISABLE_ONCE();
	return_code = deserialize_pingresp( serializer );
	ck_assert_msg( return_code == ERROR, "An error should be generated.\n");

	serializer->stream.data = reference_message_1;
	serializer->stream_index = serializer->stream.data;
	serializer->stream.length = 2;
	return_code = deserialize_pingresp( serializer );
	ck_assert_msg( return_code == SUCCES, "No error should be generated.\n");
	ck_assert_msg( serializer->message_type == 13, "The message type is not set correctly.\n");
	ck_assert_msg( serializer->stream.length == 2, "The generated output is not of the expected length.\n");

	pico_mqtt_serializer_destroy( serializer );
}
END

START(pico_mqtt_serialize_test)
{
	int error = 0;
	uint8_t index = 0;
	int return_code = 0;
	struct pico_mqtt_serializer* serializer = NULL;
	int expected_values[16] = 
	{
		ERROR,	// 00 - RESERVED    
		ERROR,	// 01 - CONNECT     
		ERROR,	// 02 - CONNACK     
		ERROR,	// 03 - PUBLISH     
		SUCCES,	// 04 - PUBACK      
		SUCCES,	// 05 - PUBREC      
		SUCCES,	// 06 - PUBREL      
		SUCCES,	// 07 - PUBCOMP     
		ERROR,	// 08 - SUBSCRIBE   
		ERROR,	// 09 - SUBACK      
		ERROR,	// 10 - UNSUBSCRIBE 
		ERROR,	// 11 - UNSUBACK    
		SUCCES,	// 12 - PINGREQ     
		SUCCES,	// 13 - PINGRESP    
		SUCCES,	// 14 - DISCONNECT  
		ERROR	// 15 - RESERVED    
	};

	PERROR_DISABLE();

	serializer = pico_mqtt_serializer_create( &error );
	pico_mqtt_serializer_clear( serializer );

	for(index = 0; index < 16; ++index)
	{
		serializer->message_type = index;
		return_code = pico_mqtt_serialize(serializer);
		ck_assert_msg( return_code == expected_values[index], "Not the expected return code (message type: %d).\n", index);
		pico_mqtt_serializer_clear(serializer);
	}

	pico_mqtt_serializer_destroy( serializer );
}
END

Suite * functional_list_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("MQTT serializer");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, compare_arrays_test);
	tcase_add_test(test_case_core, debug_malloc_test);
	tcase_add_test(test_case_core, create_serializer);
	tcase_add_test(test_case_core, clear_serializer);
	tcase_add_test(test_case_core, destroy_stream);
	tcase_add_test(test_case_core, destroy_serializer);
	tcase_add_test(test_case_core, create_stream_test);
	tcase_add_test(test_case_core, bytes_left_stream);
	tcase_add_test(test_case_core, add_byte_stream_test);
	tcase_add_test(test_case_core, get_byte_stream_test);
	tcase_add_test(test_case_core, add_2_byte_stream_test);
	tcase_add_test(test_case_core, get_2_byte_stream_test);
	tcase_add_test(test_case_core, add_data_stream_test);
	tcase_add_test(test_case_core, get_data_stream_test);
	tcase_add_test(test_case_core, add_data_and_length_stream_test);
	tcase_add_test(test_case_core, get_data_and_length_stream_test);
	tcase_add_test(test_case_core, add_array_stream_test);
	tcase_add_test(test_case_core, skip_length_test);
	tcase_add_test(test_case_core, serialize_acknowledge_test);
	tcase_add_test(test_case_core, serialize_puback_test);
	tcase_add_test(test_case_core, serialize_pubrec_test);
	tcase_add_test(test_case_core, serialize_pubrel_test);
	tcase_add_test(test_case_core, serialize_pubcomp_test);
	tcase_add_test(test_case_core, serialize_length);
	tcase_add_test(test_case_core, deserialize_length);
	tcase_add_test(test_case_core, pico_mqtt_serializer_set_message_type_test);
	tcase_add_test(test_case_core, serializer_set_client_id);
	tcase_add_test(test_case_core, serializer_set_username);
	tcase_add_test(test_case_core, serializer_set_password);
	tcase_add_test(test_case_core, serializer_set_message_test);
	tcase_add_test(test_case_core, pico_mqtt_serializer_set_keep_alive_time_test);
	tcase_add_test(test_case_core, serialize_connect_check);
	tcase_add_test(test_case_core, serialize_connect_get_flags_test);
	tcase_add_test(test_case_core, serialize_connect_get_length_test);
	tcase_add_test(test_case_core, serialize_connect_test);
	tcase_add_test(test_case_core, deserialize_connack_test);
	tcase_add_test(test_case_core, serialize_publish_test);
	tcase_add_test(test_case_core, deserialize_puback_test);
	tcase_add_test(test_case_core, deserialize_pubrec_test);
	tcase_add_test(test_case_core, deserialize_pubrel_test);
	tcase_add_test(test_case_core, deserialize_pubcomp_test);
	tcase_add_test(test_case_core, deserialize_unsuback_test);
	tcase_add_test(test_case_core, deserialize_suback_test);
	tcase_add_test(test_case_core, deserialize_publish_test);
	tcase_add_test(test_case_core, serialize_subscribe_test);
	tcase_add_test(test_case_core, serialize_unsubscribe_test);
	tcase_add_test(test_case_core, serialize_subscribtion_test);
	tcase_add_test(test_case_core, serialize_pingreq_test);
	tcase_add_test(test_case_core, serialize_pingresp_test);
	tcase_add_test(test_case_core, serialize_disconnect_test);
	tcase_add_test(test_case_core, deserialize_pingreq_test);
	tcase_add_test(test_case_core, deserialize_pingresp_test);
	tcase_add_test(test_case_core, pico_mqtt_serialize_test);

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
		if(*((uint8_t*)a + index) != *((uint8_t*)b + index))
			return 0;
	}

	return 1;
}
