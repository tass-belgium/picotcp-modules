/*#define _BSD_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>*/
#define DEBUG 3
#define DISABLE_TRACE

#include <check.h>
#include "pico_mqtt_socket_mock.h"
#include "pico_mqtt_serializer_mock.h"

/**
* test functions prototypes
**/

Suite * stream_test_suite(void);

/**
* file under test
**/

#include "pico_mqtt_stream.c"

/**
* tests
**/


START_TEST(pico_mqtt_stream_create_test)
{
	int error;
	struct pico_mqtt_stream* stream;

	MALLOC_SUCCEED();
	stream = pico_mqtt_stream_create( &error );
	ck_assert_msg( stream != NULL , "Should be able to create the stream succesfully\n");
	FREE(stream->socket);
	FREE(stream);
	CHECK_NO_ALLOCATIONS();

	MALLOC_FAIL();
	stream = pico_mqtt_stream_create( &error );
	ck_assert_msg( stream == NULL , "Should not be able to create the stream succesfully\n");
	CHECK_NO_ALLOCATIONS();

	MALLOC_SUCCEED();
	socket_mock_set_create_fail(1);
	stream = pico_mqtt_stream_create( &error );
	ck_assert_msg( stream == NULL , "Should not be able to create the stream succesfully\n");
	CHECK_NO_ALLOCATIONS();


	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_stream_destroy_test)
{
	int error;
	struct pico_mqtt_stream* stream;

	pico_mqtt_stream_destroy( NULL );

	stream = pico_mqtt_stream_create( &error );
	pico_mqtt_stream_destroy( stream );
	CHECK_NO_ALLOCATIONS();

	stream = pico_mqtt_stream_create( &error );
	stream->send_message.data = MALLOC(1);
	stream->send_message.length = 1;
	pico_mqtt_stream_destroy( stream );
	CHECK_NO_ALLOCATIONS();

	stream = pico_mqtt_stream_create( &error );
	stream->receive_message.data = MALLOC(1);
	stream->receive_message.length = 1;
	pico_mqtt_stream_destroy( stream );
	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(update_time_left_test)
{
	uint64_t previous_time = 0;
	uint64_t time_left = 100;

	previous_time = 0;
	time_left = 100;
	socket_mock_set_time( 0 );
	update_time_left( &previous_time, &time_left);
	ck_assert_msg( previous_time == 0 , "The previous_time was set incorrectly\n");
	ck_assert_msg( time_left == 100 , "The time_left was set incorrectly, time past: 0\n");

	previous_time = 0;
	time_left = 100;
	socket_mock_set_time( 50 );
	update_time_left( &previous_time, &time_left);
	ck_assert_msg( previous_time == 50 , "The previous_time was set incorrectly\n");
	ck_assert_msg( time_left == 50 , "The time_left was set incorrectly, time past: 50\n");

	previous_time = 50;
	time_left = 100;
	socket_mock_set_time( 50 );
	update_time_left( &previous_time, &time_left);
	ck_assert_msg( previous_time == 50 , "The previous_time was set incorrectly\n");
	ck_assert_msg( time_left == 100 , "The time_left was set incorrectly, time past: 50\n");

	previous_time = 50;
	time_left = 100;
	socket_mock_set_time( 150 );
	update_time_left( &previous_time, &time_left);
	ck_assert_msg( previous_time == 150 , "The previous_time was set incorrectly\n");
	ck_assert_msg( time_left == 0 , "The time_left was set incorrectly, time past: 100\n");

	previous_time = 50;
	time_left = 100;
	socket_mock_set_time( 200 );
	update_time_left( &previous_time, &time_left);
	ck_assert_msg( previous_time == 200 , "The previous_time was set incorrectly\n");
	ck_assert_msg( time_left == 0 , "The time_left was set incorrectly, time past: 150\n");

	CHECK_NO_ALLOCATIONS();
}
END_TEST


START_TEST(pico_mqtt_stream_connect_test)
{
	int result = 0;
	int error = 0;
	struct pico_mqtt_stream* stream = NULL;
	stream = pico_mqtt_stream_create( &error );



	socket_mock_set_return_value(SUCCES);
	result = pico_mqtt_stream_connect( stream, NULL, NULL);
	ck_assert_msg( result == SUCCES , "The call should have succeedded\n");

	PERROR_DISABLE_ONCE();
	socket_mock_set_return_value(ERROR);
	result = pico_mqtt_stream_connect( stream, NULL, NULL);
	ck_assert_msg( result == ERROR , "The call should not have succeedded\n");

	pico_mqtt_stream_destroy( stream );
	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_stream_is_message_sending_test)
{
	int result = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;

	result = pico_mqtt_stream_is_message_sending( &stream );
	ck_assert_msg( result == 0 , "No message is being send, incorrect result\n");

	stream.send_message.data = &result;
	result = pico_mqtt_stream_is_message_sending( &stream );
	ck_assert_msg( result == 1 , "A message is being send, incorrect result\n");

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_stream_is_message_received_test)
{
	int result = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;

	result = pico_mqtt_stream_is_message_received( &stream );
	ck_assert_msg( result == 0 , "No message is received yet, incorrect result\n");

	stream.receive_message.data = &result;
	stream.receive_message_buffer.length = 1;
	result = pico_mqtt_stream_is_message_received( &stream );
	ck_assert_msg( result == 0 , "No message is received yet, incorrect result\n");

	stream.receive_message.data = &result;
	stream.receive_message_buffer.length = 0;
	result = pico_mqtt_stream_is_message_received( &stream );
	ck_assert_msg( result == 1 , "A message is received, incorrect result\n");

	stream.receive_message.data = NULL;
	stream.receive_message_buffer.length = 1;
	result = pico_mqtt_stream_is_message_received( &stream );
	ck_assert_msg( result == 0 , "No message is received yet, incorrect result\n");

	CHECK_NO_ALLOCATIONS();
}
END_TEST


START_TEST(pico_mqtt_stream_set_send_message_test)
{
	int result = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;
	struct pico_mqtt_data data = PICO_MQTT_DATA_EMPTY;
	data.data = &result;
	data.length = 1;

	pico_mqtt_stream_set_send_message( &stream, data);
	ck_assert_msg( stream.send_message.data == &result , "data was set incorrectly\n");
	ck_assert_msg( stream.send_message.length == 1 , "data was set incorrectly\n");
	ck_assert_msg( stream.send_message_buffer.data == &result , "data was set incorrectly\n");
	ck_assert_msg( stream.send_message_buffer.length == 1 , "data was set incorrectly\n");

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_stream_get_received_message_test)
{
	int result = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;
	struct pico_mqtt_data data = PICO_MQTT_DATA_EMPTY;
	struct pico_mqtt_data data_received = PICO_MQTT_DATA_EMPTY;
	data.data = &result;
	data.length = 1;
	stream.receive_message = data;

	data_received = pico_mqtt_stream_get_received_message( &stream );
	ck_assert_msg( data_received.data == data.data , "data was set incorrectly\n");
	ck_assert_msg( data_received.length == data.length , "data was set incorrectly\n");
	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(receiving_fixed_header_test)
{
	int result = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;
	
	stream.fixed_header_next_byte = 1;
	result = receiving_fixed_header( &stream );
	ck_assert_msg( result == 1 , "Reading fixed header not detected\n");

	stream.fixed_header_next_byte = 0;
	result = receiving_fixed_header( &stream );
	ck_assert_msg( result == 0 , "Not reading fixed header, false detected\n");

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(receiving_payload_test)
{
	int result = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;
	
	stream.receive_message_buffer.length = 1;
	result = receiving_payload( &stream );
	ck_assert_msg( result == 1 , "Reading payload not detected\n");

	stream.receive_message_buffer.length = 0;
	result = receiving_payload( &stream );
	ck_assert_msg( result == 0 , "Not reading payload, false detected\n");

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(receive_in_progress_test)
{
	int result = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;

	stream.fixed_header_next_byte = 0;
	stream.receive_message_buffer.length = 0;
	result = receive_in_progress( &stream );
	ck_assert_msg( result == 0 , "Inactive input falsly detected.");

	stream.fixed_header_next_byte = 0;
	stream.receive_message_buffer.length = 1;
	result = receive_in_progress( &stream );
	ck_assert_msg( result == 1 , "Active input not detected.");
	
	stream.fixed_header_next_byte = 1;
	stream.receive_message_buffer.length = 0;
	result = receive_in_progress( &stream );
	ck_assert_msg( result == 1 , "Active input not detected.");

	stream.fixed_header_next_byte = 1;
	stream.receive_message_buffer.length = 1;
	result = receive_in_progress( &stream );
	ck_assert_msg( result == 1 , "Active input not detected.");

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(is_fixed_header_complete_test)
{
	int result = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;

	stream.fixed_header_next_byte = 0;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 0, "fixed header completion check failed\n");

	stream.fixed_header_next_byte = 1;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 0, "fixed header completion check failed\n");


	stream.fixed_header_next_byte = 5;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 1, "fixed header completion check failed\n");

	stream.fixed_header_next_byte = 2;
	stream.fixed_header[0] = 0;
	stream.fixed_header[1] = 0x00;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 1, "fixed header completion check failed\n");

	stream.fixed_header_next_byte = 2;
	stream.fixed_header[0] = 0;
	stream.fixed_header[1] = 0x80;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 0, "fixed header completion check failed\n");

	stream.fixed_header_next_byte = 3;
	stream.fixed_header[0] = 0;
	stream.fixed_header[1] = 0x80;
	stream.fixed_header[2] = 0x00;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 1, "fixed header completion check failed\n");

	stream.fixed_header_next_byte = 3;
	stream.fixed_header[0] = 0;
	stream.fixed_header[1] = 0x80;
	stream.fixed_header[2] = 0x80;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 0, "fixed header completion check failed\n");

	stream.fixed_header_next_byte = 4;
	stream.fixed_header[0] = 0;
	stream.fixed_header[1] = 0x80;
	stream.fixed_header[2] = 0x80;
	stream.fixed_header[3] = 0x00;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 1, "fixed header completion check failed\n");

	stream.fixed_header_next_byte = 4;
	stream.fixed_header[0] = 0;
	stream.fixed_header[1] = 0x80;
	stream.fixed_header[2] = 0x80;
	stream.fixed_header[3] = 0x80;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 0, "fixed header completion check failed\n");

	stream.fixed_header_next_byte = 5;
	stream.fixed_header[0] = 0;
	stream.fixed_header[1] = 0x80;
	stream.fixed_header[2] = 0x80;
	stream.fixed_header[3] = 0x80;
	stream.fixed_header[4] = 0x00;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 1, "fixed header completion check failed\n");

	stream.fixed_header_next_byte = 5;
	stream.fixed_header[0] = 0;
	stream.fixed_header[1] = 0x80;
	stream.fixed_header[2] = 0x80;
	stream.fixed_header[3] = 0x80;
	stream.fixed_header[4] = 0x80;
	result = is_fixed_header_complete( &stream );
	ck_assert_msg(result == 1, "fixed header completion check failed\n");



	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(is_send_receive_done_test)
{
	int result = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;

	stream.fixed_header_next_byte = 0;
	stream.receive_message.data = NULL;
	stream.send_message.data = NULL;
	result = is_send_receive_done( &stream, 0 );
	ck_assert_msg( result == 1 , "send receive detection failed.");

	stream.fixed_header_next_byte = 0;
	stream.receive_message.data = NULL;
	stream.send_message.data = NULL;
	result = is_send_receive_done( &stream, 1 );
	ck_assert_msg( result == 0 , "send receive detection failed.");

	stream.fixed_header_next_byte = 0;
	stream.receive_message.data = NULL;
	stream.send_message.data = &result;
	result = is_send_receive_done( &stream, 0 );
	ck_assert_msg( result == 0 , "send receive detection failed.");

	stream.fixed_header_next_byte = 1;
	stream.receive_message.data = NULL;
	stream.send_message.data = NULL;
	result = is_send_receive_done( &stream, 0 );
	ck_assert_msg( result == 0 , "send receive detection failed.");

	stream.fixed_header_next_byte = 0;
	stream.receive_message.data = &result;
	stream.send_message.data = NULL;
	result = is_send_receive_done( &stream, 0 );
	ck_assert_msg( result == 1 , "send receive detection failed.");

	stream.fixed_header_next_byte = 0;
	stream.receive_message.data = &result;
	stream.send_message.data = NULL;
	result = is_send_receive_done( &stream, 1 );
	ck_assert_msg( result == 1 , "send receive detection failed.");

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(process_fixed_header_test)
{
	int result = 0;
	int error = 0;
	struct pico_mqtt_stream stream = PICO_MQTT_STREAM_EMPTY;
	stream.error = &error;
	stream.fixed_header[0] = 0;
	stream.fixed_header[1] = 1;
	stream.fixed_header[2] = 2;
	stream.fixed_header[3] = 3;
	stream.fixed_header[4] = 4;

	serializer_mock_reset();
	stream.fixed_header_next_byte = 0;
	result = process_fixed_header( &stream );
	ck_assert_msg( result == SUCCES , "Nothing should happen, fixed_header not yet complete");
	ck_assert_msg( stream.receive_message.data == NULL , "Nothing should happen, fixed_header not yet complete");

	stream.fixed_header_next_byte = 5;
	serializer_mock_set_return_value( ERROR );
	result = process_fixed_header( &stream );
	ck_assert_msg( result == ERROR , "Nothing should happen, the fixed header has an error");
	ck_assert_msg( stream.receive_message.data == NULL , "Nothing should happen, the fixed header has an error");

	PERROR_DISABLE();
	stream.fixed_header_next_byte = 5;
	serializer_mock_set_return_value( SUCCES );
	serializer_mock_set_length(MAXIMUM_MESSAGE_SIZE - 3);
	result = process_fixed_header( &stream );
	ck_assert_msg( result == ERROR , "Nothing should happen, the message is to long");
	ck_assert_msg( stream.receive_message.data == NULL , "Nothing should happen, the message is to long");

	MALLOC_FAIL_ONCE();
	stream.fixed_header_next_byte = 5;
	serializer_mock_set_return_value( SUCCES );
	serializer_mock_set_length(MAXIMUM_MESSAGE_SIZE-5);
	result = process_fixed_header( &stream );
	ck_assert_msg( result == ERROR , "Nothing should happen, allocating the massage failed");
	ck_assert_msg( stream.receive_message.data == NULL , "Nothing should happen, allocating the massage failed");
	ck_assert_msg( stream.receive_message.length == 0 , "Nothing should happen, allocating the massage failed");
	PERROR_ENABLE();

	stream.fixed_header_next_byte = 5;
	serializer_mock_set_return_value( SUCCES );
	serializer_mock_set_length(MAXIMUM_MESSAGE_SIZE-5);
	result = process_fixed_header( &stream );
	ck_assert_msg( result == SUCCES , "The fixed header should be processed correctly");
	ck_assert_msg( stream.receive_message.data != NULL , "The fixed header should be processed correctly");
	ck_assert_msg( stream.receive_message.length == MAXIMUM_MESSAGE_SIZE , "The fixed header should be processed correctly");
	ck_assert_msg( stream.receive_message_buffer.data == ((uint8_t*)(stream.receive_message.data)) + 5 , "The fixed header should be processed correctly");
	ck_assert_msg( stream.receive_message_buffer.length == stream.receive_message.length-5, "The fixed header should be processed correctly");
	ck_assert_msg( stream.fixed_header_next_byte == 0 , "The fixed header should be processed correctly");
	ck_assert_msg( ((uint8_t*)(stream.receive_message.data))[0] == 0 , "The fixed header should be processed correctly");
	ck_assert_msg( ((uint8_t*)(stream.receive_message.data))[1] == 1, "The fixed header should be processed correctly");
	ck_assert_msg( ((uint8_t*)(stream.receive_message.data))[2] == 2 , "The fixed header should be processed correctly");
	ck_assert_msg( ((uint8_t*)(stream.receive_message.data))[3] == 3 , "The fixed header should be processed correctly");
	ck_assert_msg( ((uint8_t*)(stream.receive_message.data))[4] == 4 , "The fixed header should be processed correctly");

	FREE(stream.receive_message.data);

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(send_receive_message_test)
{
	int result = 0;
	uint8_t buffer_result[4] = {0,0,0,0};
	int error = 0;
	struct pico_mqtt_stream* stream = pico_mqtt_stream_create( &error );

	// message is received
	stream->receive_message.data = buffer_result;
	stream->receive_message_buffer.length = 0;
	socket_mock_set_return_value( ERROR );
	result = send_receive_message( stream, 1);
	ck_assert_msg( result == ERROR , "Sending a message should fail\n");

	// message is received
	stream->receive_message_buffer.length = 0;
	socket_mock_set_return_value( SUCCES );
	result = send_receive_message( stream, 1);
	ck_assert_msg( result == SUCCES , "Sending a message should succeed\n");

	// fixed header is received
	stream->receive_message.data = buffer_result;
	stream->receive_message_buffer.length = 1;
	stream->fixed_header_next_byte = 0;
	socket_mock_set_return_value( ERROR );
	result = send_receive_message( stream, 1);
	ck_assert_msg( result == ERROR , "Sending a message should fail\n");

	// fixed header is received
	socket_mock_set_bytes_to_receive(4);
	stream->receive_message_buffer.data = buffer_result;
	stream->receive_message_buffer.length = 4;
	stream->fixed_header_next_byte = 0;
	socket_mock_set_return_value( SUCCES );
	result = send_receive_message( stream, 1);
	ck_assert_msg( result == SUCCES , "Sending a message should succeed\n");

	ck_assert_msg( buffer_result[0] == 0, "The receive_message is incorrect\n");
	ck_assert_msg( buffer_result[1] == 1, "The receive_message is incorrect\n");
	ck_assert_msg( buffer_result[2] == 2, "The receive_message is incorrect\n");
	ck_assert_msg( buffer_result[3] == 3, "The receive_message is incorrect\n");

	// fixed header is being received
	socket_mock_set_bytes_to_receive(0);
	stream->receive_message_buffer.data = buffer_result;
	stream->receive_message_buffer.length = 0;
	stream->fixed_header_next_byte = 1;
	stream->fixed_header[0] = 0;
	stream->fixed_header[1] = 0;
	socket_mock_set_return_value( ERROR );
	result = send_receive_message( stream, 1);
	ck_assert_msg( result == ERROR , "Sending a message should fail\n");

	socket_mock_set_bytes_to_receive(1);
	stream->receive_message.data = NULL;
	stream->receive_message_buffer.data = NULL;
	stream->receive_message_buffer.length = 0;
	stream->fixed_header_next_byte = 1;
	socket_mock_set_return_value( SUCCES );
	serializer_mock_set_length(1);
	result = send_receive_message( stream, 1);
	ck_assert_msg( result == SUCCES , "Sending a message should succeed\n");
	ck_assert_msg( stream->fixed_header_next_byte == 0, "Did not set the fixed header next byte to 0.\n");
	FREE(stream->receive_message.data);

	socket_mock_set_bytes_to_receive(1);
	stream->receive_message.data = NULL;
	stream->receive_message_buffer.data = NULL;
	stream->receive_message_buffer.length = 0;
	stream->fixed_header_next_byte = 1;
	socket_mock_set_return_value( ERROR );
	serializer_mock_set_length(1);
	result = send_receive_message( stream, 1);
	ck_assert_msg( result == ERROR , "Sending a message should fail\n");
	ck_assert_msg( stream->fixed_header_next_byte == 1, "Did not set the fixed header next byte to 0.\n");

	socket_mock_set_bytes_to_receive(1);
	stream->receive_message.data = NULL;
	stream->receive_message_buffer.data = NULL;
	stream->receive_message_buffer.length = 0;
	stream->fixed_header_next_byte = 5;
	socket_mock_set_return_value( SUCCES );
	serializer_mock_set_length(1);
	serializer_mock_set_return_value(ERROR);
	result = send_receive_message( stream, 1);
	ck_assert_msg( result == ERROR , "Sending a message should fail\n");
	ck_assert_msg( stream->fixed_header_next_byte == 6, "Did not set the fixed header next byte to 0.\n");

	
	stream->receive_message.data = NULL;
	pico_mqtt_stream_destroy( stream );
	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_stream_send_receive_test)
{
	int result = 0;
	int error = 0;
	uint64_t time_left = 10;
	struct pico_mqtt_stream* stream = pico_mqtt_stream_create( &error );
	socket_mock_reset();
	serializer_mock_reset();

	socket_mock_set_time_interval( 1 );
	//message is received
	stream->receive_message.data = &result;
	stream->receive_message_buffer.length = 0;
	result = pico_mqtt_stream_send_receive( stream, &time_left, 0);
	ck_assert_msg( result == SUCCES , "Sending a message should succeedded\n");
	ck_assert_msg( time_left == 10, "No time should have past\n ");
	stream->receive_message.data = NULL;

	//message is received
	stream->receive_message.data = &result;
	stream->receive_message_buffer.length = 0;
	stream->send_message_buffer.length = 1;
	stream->send_message_buffer.data = &result;
	stream->send_message.data = &result;
	result = pico_mqtt_stream_send_receive( stream, &time_left, 0);
	ck_assert_msg( result == SUCCES , "Sending a message should succeedded\n");
	ck_assert_msg( time_left == 0, "Time should have past\n ");
	stream->receive_message.data = NULL;
	stream->send_message_buffer.data = NULL;
	stream->send_message.data = NULL;

	pico_mqtt_stream_destroy( stream );
	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_stream_send_receive_fail_test)
{
	int result = 0;
	int error = 0;
	uint64_t time_left = 10;
	struct pico_mqtt_stream* stream = pico_mqtt_stream_create( &error );
	socket_mock_reset();
	serializer_mock_reset();

	socket_mock_set_time_interval( 1 );
	stream->receive_message_buffer.data = &result;
	stream->receive_message_buffer.length = 0;
	stream->send_message_buffer.length = 1;
	stream->send_message_buffer.data = &result;
	stream->send_message.data = &result;
	stream->receive_message.data = &result;
	socket_mock_set_return_value(ERROR);
	result = pico_mqtt_stream_send_receive( stream, &time_left, 0);
	ck_assert_msg( result == ERROR , "Sending a message should fail\n");
	ck_assert_msg( time_left == 10, "Time should have past\n ");
	stream->receive_message.data = NULL;
	stream->send_message_buffer.data = NULL;
	stream->send_message.data = NULL;
	stream->receive_message_buffer.data = NULL;

	pico_mqtt_stream_destroy( stream );
	CHECK_NO_ALLOCATIONS();
}
END_TEST

Suite * stream_test_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("MQTT stream");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, pico_mqtt_stream_create_test);
	tcase_add_test(test_case_core, pico_mqtt_stream_destroy_test);
	tcase_add_test(test_case_core, update_time_left_test);
	tcase_add_test(test_case_core, pico_mqtt_stream_connect_test);
	tcase_add_test(test_case_core, pico_mqtt_stream_is_message_sending_test);
	tcase_add_test(test_case_core, pico_mqtt_stream_is_message_received_test);
	tcase_add_test(test_case_core, pico_mqtt_stream_set_send_message_test);
	tcase_add_test(test_case_core, pico_mqtt_stream_get_received_message_test);
	tcase_add_test(test_case_core, receiving_fixed_header_test);
	tcase_add_test(test_case_core, receiving_payload_test);
	tcase_add_test(test_case_core, receive_in_progress_test);
	tcase_add_test(test_case_core, is_fixed_header_complete_test);
	tcase_add_test(test_case_core, is_send_receive_done_test);
	tcase_add_test(test_case_core, process_fixed_header_test);
	tcase_add_test(test_case_core, send_receive_message_test);
	tcase_add_test(test_case_core, pico_mqtt_stream_send_receive_test);
	tcase_add_test(test_case_core, pico_mqtt_stream_send_receive_fail_test);

	suite_add_tcase(test_suite, test_case_core);

	return test_suite;
}

 int main(void)
 {
	int number_failed;
	Suite *test_suite;
	SRunner *suite_runner;

	test_suite = stream_test_suite();
	suite_runner = srunner_create(test_suite);

	srunner_run_all(suite_runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(suite_runner);
	srunner_free(suite_runner);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }

/**
* test functions implementation
**/
