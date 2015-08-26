#define _BSD_SOURCE
#include <stdlib.h>
#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

/**
* test data types
**/

struct pico_mqtt{
	uint32_t bytes_used;
	struct pico_mqtt_socket* socket;
};

struct pico_mqtt_socket{
	uint32_t bytes_to_read;
	uint32_t bytes_to_write;
	uint32_t usecs_to_use;
	uint32_t secs_to_use;
	int value_to_return;
};

/**
* test functions prototypes
**/

int my_malloc(struct pico_mqtt* mqtt, void** data, size_t size);
int my_free(struct pico_mqtt* mqtt, void* data, size_t size);

/**
* file under test
**/

#include "pico_mqtt_stream.c"

/**
* tests
**/

START_TEST(moc_socket_test)
{
	uint8_t indexes[10] = {0,1,2,3,4,5,6,7,8,9};
	struct pico_mqtt_data read_buffer = {
		.length = 10,
		.data = (void*) indexes};
	struct pico_mqtt_data write_buffer = {
		.length = 10,
		.data = (void*) indexes};
	struct pico_mqtt_socket orders = {
		.bytes_to_read = 0,
		.bytes_to_write = 0,
		.value_to_return = 0
	};
	struct timeval time_left = {
		.tv_usec = 0,
		.tv_sec = 0
	};
	int result;
	
	result = pico_mqtt_connection_send_receive(&orders, &read_buffer, &write_buffer, &time_left);
	ck_assert_msg( result == 0, "Moc socket does not return requested result (0)\n");
	ck_assert_msg( write_buffer.data == indexes, "Moc socket changes write buffer index when no change is asked\n");
	ck_assert_msg( write_buffer.length == 10, "Moc socket changes write buffer length left when no change is asked\n");
	ck_assert_msg( read_buffer.data == indexes, "Moc socket changes read buffer index when no change is asked\n");
	ck_assert_msg( read_buffer.length == 10, "Moc socket changes read buffer length left when no change is asked\n");
	ck_assert_msg( time_left.tv_usec == 0, "The time left should be the same (0 microsecondes).");
	ck_assert_msg( time_left.tv_sec == 0, "The time left should be the same (0 seconds).");

	read_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	write_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	orders.bytes_to_read = 1;
	orders.bytes_to_write = 0;
	orders.value_to_return = 1;

	result = pico_mqtt_connection_send_receive(&orders, &read_buffer, &write_buffer, &time_left);
	ck_assert_msg( result == 1, "Moc socket does not return requested result (1)\n");
	ck_assert_msg( write_buffer.data == indexes, "Moc socket changes write buffer index when no change is asked\n");
	ck_assert_msg( write_buffer.length == 10, "Moc socket changes write buffer length left when no change is asked\n");
	ck_assert_msg( read_buffer.data == &(indexes[1]), "Moc socket did not read the correct number of bytes\n");
	ck_assert_msg( read_buffer.length == 9, "Moc socket didn't changed the read buffer length as requested\n");
	ck_assert_msg( time_left.tv_usec == 0, "The time left should be the same (0 microsecondes).");
	ck_assert_msg( time_left.tv_sec == 0, "The time left should be the same (0 seconds).");


	read_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	write_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	orders.bytes_to_read = 0;
	orders.bytes_to_write = 3;
	orders.value_to_return = 2;

	result = pico_mqtt_connection_send_receive(&orders, &read_buffer, &write_buffer, &time_left);
	ck_assert_msg( result == 2, "Moc socket does not return requested result (2)\n");
	ck_assert_msg( write_buffer.data == &(indexes[3]), "Moc socket did not write the correct number of bytes\n");
	ck_assert_msg( write_buffer.length == 7, "Moc socket didn't changed the write buffer length as requested\n");
	ck_assert_msg( read_buffer.data == indexes, "Moc socket changes read buffer index when no change is asked\n");
	ck_assert_msg( read_buffer.length == 10, "Moc socket changes read buffer length left when no change is asked\n");
	ck_assert_msg( time_left.tv_usec == 0, "The time left should be the same (0 microsecondes).");
	ck_assert_msg( time_left.tv_sec == 0, "The time left should be the same (0 seconds).");


	read_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	write_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	orders.bytes_to_read = 7;
	orders.bytes_to_write = 5;
	orders.value_to_return = 0;

	result = pico_mqtt_connection_send_receive(&orders, &read_buffer, &write_buffer, &time_left);
	ck_assert_msg( result == 0, "Moc socket does not return requested result (0)\n");
	ck_assert_msg( write_buffer.data == &(indexes[5]), "Moc socket did not write the correct number of bytes\n");
	ck_assert_msg( write_buffer.length == 5, "Moc socket didn't changed the write buffer length as requested\n");
	ck_assert_msg( read_buffer.data == &(indexes[7]), "Moc socket did not read the correct number of bytes\n");
	ck_assert_msg( read_buffer.length == 3, "Moc socket didn't changed the read buffer length as requested\n");
	ck_assert_msg( time_left.tv_usec == 0, "The time left should be the same (0 microsecondes).");
	ck_assert_msg( time_left.tv_sec == 0, "The time left should be the same (0 seconds).");

	read_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	write_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	time_left = (struct timeval){.tv_usec = 0, .tv_sec = 0};
	orders.bytes_to_read = 4;
	orders.bytes_to_write = 3;
	orders.value_to_return = 2;
	orders.usecs_to_use = 1;
	orders.secs_to_use = 0;

	result = pico_mqtt_connection_send_receive(&orders, &read_buffer, &write_buffer, &time_left);
	ck_assert_msg( result == 2, "Moc socket does not return requested result (2)\n");
	ck_assert_msg( write_buffer.data == &(indexes[3]), "Moc socket did not write the correct number of bytes\n");
	ck_assert_msg( write_buffer.length == 7, "Moc socket didn't changed the write buffer length as requested\n");
	ck_assert_msg( read_buffer.data == &(indexes[4]), "Moc socket did not read the correct number of bytes\n");
	ck_assert_msg( read_buffer.length == 6, "Moc socket didn't changed the read buffer length as requested\n");
	ck_assert_msg( time_left.tv_usec == 0, "The time left should be 0 microsecondes, can not have a negative time.");
	ck_assert_msg( time_left.tv_sec == 0, "The time left should be the same (0 seconds).");

	read_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	write_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	time_left = (struct timeval){.tv_usec = 0, .tv_sec = 0};
	orders.bytes_to_read = 4;
	orders.bytes_to_write = 3;
	orders.value_to_return = 2;
	orders.usecs_to_use = 0;
	orders.secs_to_use = 1;

	result = pico_mqtt_connection_send_receive(&orders, &read_buffer, &write_buffer, &time_left);
	ck_assert_msg( result == 2, "Moc socket does not return requested result (2)\n");
	ck_assert_msg( write_buffer.data == &(indexes[3]), "Moc socket did not write the correct number of bytes\n");
	ck_assert_msg( write_buffer.length == 7, "Moc socket didn't changed the write buffer length as requested\n");
	ck_assert_msg( read_buffer.data == &(indexes[4]), "Moc socket did not read the correct number of bytes\n");
	ck_assert_msg( read_buffer.length == 6, "Moc socket didn't changed the read buffer length as requested\n");
	ck_assert_msg( time_left.tv_usec == 0, "The time left should be 0 microsecondes, can not have a negative time.");
	ck_assert_msg( time_left.tv_sec == 0, "The time left should be the same 0 seconds, can not have a negative time.");

	read_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	write_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	time_left = (struct timeval){.tv_usec = 10, .tv_sec = 10};
	orders.bytes_to_read = 4;
	orders.bytes_to_write = 3;
	orders.value_to_return = 2;
	orders.usecs_to_use = 5;
	orders.secs_to_use = 5;

	result = pico_mqtt_connection_send_receive(&orders, &read_buffer, &write_buffer, &time_left);
	ck_assert_msg( result == 2, "Moc socket does not return requested result (2)\n");
	ck_assert_msg( write_buffer.data == &(indexes[3]), "Moc socket did not write the correct number of bytes\n");
	ck_assert_msg( write_buffer.length == 7, "Moc socket didn't changed the write buffer length as requested\n");
	ck_assert_msg( read_buffer.data == &(indexes[4]), "Moc socket did not read the correct number of bytes\n");
	ck_assert_msg( read_buffer.length == 6, "Moc socket didn't changed the read buffer length as requested\n");
	ck_assert_msg( time_left.tv_usec == 5, "The time left should be 5 microsecondes.");
	ck_assert_msg( time_left.tv_sec == 5, "The time left should be the 5 seconds.");

	read_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	write_buffer = (struct pico_mqtt_data){.length = 10, .data = (void*) indexes};
	time_left = (struct timeval){.tv_usec = 0, .tv_sec = 10};
	orders.bytes_to_read = 4;
	orders.bytes_to_write = 3;
	orders.value_to_return = 2;
	orders.usecs_to_use = 500000;
	orders.secs_to_use = 9;

	result = pico_mqtt_connection_send_receive(&orders, &read_buffer, &write_buffer, &time_left);
	ck_assert_msg( result == 2, "Moc socket does not return requested result (2)\n");
	ck_assert_msg( write_buffer.data == &(indexes[3]), "Moc socket did not write the correct number of bytes\n");
	ck_assert_msg( write_buffer.length == 7, "Moc socket didn't changed the write buffer length as requested\n");
	ck_assert_msg( read_buffer.data == &(indexes[4]), "Moc socket did not read the correct number of bytes\n");
	ck_assert_msg( read_buffer.length == 6, "Moc socket didn't changed the read buffer length as requested\n");
	ck_assert_msg( time_left.tv_usec == 500000, "The time left should be 500000 microsecondes.");
	ck_assert_msg( time_left.tv_sec == 0, "The time left should be the 0 seconds.");
}
END_TEST

START_TEST(create_and_destroy_test)
{
	int result = 0;
	struct pico_mqtt_stream* stream;
	struct pico_mqtt_socket socket;
	struct pico_mqtt mqtt = {.bytes_used = 0, .socket = &socket};

	result = pico_mqtt_stream_create( &mqtt, &stream, my_malloc, my_free);

	ck_assert_msg( result == 0, "Failed to create the stream\n");

	pico_mqtt_stream_destroy( stream );
}
END_TEST

START_TEST(is_output_message_set_test){
	int result = 0;
	struct pico_mqtt_stream* stream;
	struct pico_mqtt_socket socket;
	uint8_t indexes[10] = {0,1,2,3,4,5,6,7,8,9};
	struct pico_mqtt_data message = {.length = 10, .data = indexes};
	struct pico_mqtt mqtt = {.bytes_used = 0, .socket = &socket};

	result = pico_mqtt_stream_is_output_message_set( NULL );
	ck_assert_msg( result == 0, "If no stream is specified, the check function should return 0.");

	result = pico_mqtt_stream_create( &mqtt, &stream, my_malloc, my_free);
	ck_assert_msg( result == 0, "Failed to create the stream\n");

	result = pico_mqtt_stream_is_output_message_set( stream );
	ck_assert_msg( result == 0, "If no message is set, the check function should return 0.");

	stream->output_message = &message;

	result = pico_mqtt_stream_is_output_message_set( stream );
	ck_assert_msg( result == 1, "If a message is set, the check function should return 1.");
	
	pico_mqtt_stream_destroy( stream );
}
END_TEST

START_TEST(is_input_message_set_test){
	int result = 0;
	struct pico_mqtt_stream* stream;
	struct pico_mqtt_socket socket;
	uint8_t indexes[10] = {0,1,2,3,4,5,6,7,8,9};
	struct pico_mqtt_data message = {.length = 10, .data = indexes};
	struct pico_mqtt mqtt = {.bytes_used = 0, .socket = &socket};

	result = pico_mqtt_stream_is_input_message_set( NULL );
	ck_assert_msg( result == 0, "If no stream is specified, the check function should return 0.");

	result = pico_mqtt_stream_create( &mqtt, &stream, my_malloc, my_free);
	ck_assert_msg( result == 0, "Failed to create the stream\n");

	result = pico_mqtt_stream_is_input_message_set( stream );
	ck_assert_msg( result == 0, "If no message is set, the check function should return 0.");

	stream->input_message = &message;

	result = pico_mqtt_stream_is_input_message_set( stream );
	ck_assert_msg( result == 1, "If a message is set, the check function should return 1.");
	
	pico_mqtt_stream_destroy( stream );
}
END_TEST


START_TEST(set_message_test){
	int result = 0;
	struct pico_mqtt_stream* stream;
	struct pico_mqtt_socket socket;
	uint8_t indexes[10] = {0,1,2,3,4,5,6,7,8,9};
	struct pico_mqtt_data message = {.length = 10, .data = indexes};
	struct pico_mqtt mqtt = {.bytes_used = 0, .socket = &socket};

	result = pico_mqtt_stream_create( &mqtt, &stream, my_malloc, my_free);
	ck_assert_msg( result == 0, "Failed to create the stream\n");

	result = pico_mqtt_stream_set_message( stream, NULL);
	ck_assert_msg( result ==-1, "An error should be returned when giving a invalid message\n");

	result = pico_mqtt_stream_set_message( NULL, NULL);
	ck_assert_msg( result ==-1, "An error should be returned when giving an invallid stream\n");

	result = pico_mqtt_stream_set_message( stream, &message );
	ck_assert_msg( result == 0, "No error should occur when setting a valid message\n");

	result = pico_mqtt_stream_set_message( stream, &message );
	ck_assert_msg( result ==-4, "An error should occur when trying to set a message when a message was already was set.\n");
	
	pico_mqtt_stream_destroy( stream );
}
END_TEST

START_TEST(is_time_left_test)
{
	struct timeval time_left = {.tv_usec = 0, .tv_sec = 0};
	int result;

	result = is_time_left( NULL );
	ck_assert_msg( result == -1, "When time left is not specified, result should be -1");

	result = is_time_left( &time_left );
	ck_assert_msg( result == 0, "When no time is left, 0 should be returned.");

	time_left.tv_usec = 1;

	result = is_time_left( &time_left );
	ck_assert_msg( result == 1, "When 1 microsecond is left, 1 should be returned.");

	time_left.tv_usec = 0;
	time_left.tv_sec = 1;

	result = is_time_left( &time_left );
	ck_assert_msg( result == 1, "When 1 second is left, 1 should be returned.");

	time_left.tv_usec = 1;
	time_left.tv_sec = 1;

	result = is_time_left( &time_left );
	ck_assert_msg( result == 1, "When 1 second and 1 microsecond is left, 1 should be returned.");
}
END_TEST

START_TEST(send_message_test){	int result = 0;
	struct pico_mqtt_stream* stream;
	struct pico_mqtt_socket socket;
	uint8_t indexes[10] = {0,1,2,3,4,5,6,7,8,9};
	struct pico_mqtt_data message = {.length = 10, .data = indexes};
	struct timeval time_left = {.tv_sec = 0, .tv_usec = 0};
	struct pico_mqtt mqtt = {.bytes_used = 0, .socket = &socket};

	socket = (struct pico_mqtt_socket){
		.bytes_to_read = 0,
		.bytes_to_write = 0,
		.usecs_to_use = 0,
		.secs_to_use = 0,
		.value_to_return = 0
	};

	result = pico_mqtt_stream_create( &mqtt, &stream, my_malloc, my_free);
	ck_assert_msg( result == 0, "Failed to create the stream\n");

	result = pico_mqtt_stream_set_message( stream, &message);
	ck_assert_msg( result == 0, "Failed to set the message to send\n");

	result = pico_mqtt_stream_send_receive( stream, &time_left);
	ck_assert_msg( result == 0, "A timeout should have occured but didn't\n");

	socket = (struct pico_mqtt_socket) {
		.bytes_to_read = 10,
		.bytes_to_write = 0,
		.usecs_to_use = 500000,
		.secs_to_use = 2,
		.value_to_return = 0
	};
	
	time_left = (struct timeval) {.tv_sec = 5, .tv_usec = 0};

	result = pico_mqtt_stream_send_receive( stream, &time_left);
	ck_assert_msg( result == 1, "The result should be 1\n");
	ck_assert_msg( time_left.tv_sec == 2, "2 seconds should have been left\n");
	ck_assert_msg( time_left.tv_usec == 500000, "500000 usecs should have been left\n");
	ck_assert_msg( message.length == 5, "5 bytes should have been left after reading\n");
	ck_assert_msg( message.data == &(indexes[5]), "the index should have been 5\n");
	ck_assert_msg( !pico_mqtt_stream_is_input_message_set(stream), "no input message should have been set\n");

	pico_mqtt_stream_destroy( stream );
}
END_TEST

Suite * tcp_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("MQTT stream");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, moc_socket_test);
	tcase_add_test(test_case_core, create_and_destroy_test);
	tcase_add_test(test_case_core, is_output_message_set_test);
	tcase_add_test(test_case_core, is_input_message_set_test);
	tcase_add_test(test_case_core, set_message_test);
	tcase_add_test(test_case_core, is_time_left_test);
	tcase_add_test(test_case_core, send_message_test);

	suite_add_tcase(test_suite, test_case_core);

	return test_suite;
}

 int main(void)
 {
	int number_failed;
	Suite *test_suite;
	SRunner *suite_runner;

	test_suite = tcp_suite();
	suite_runner = srunner_create(test_suite);

	srunner_run_all(suite_runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(suite_runner);
	srunner_free(suite_runner);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }

/**
* test functions implementation
**/

int my_malloc(struct pico_mqtt* mqtt, void** data, size_t size){
	void* result;
	
	if(size <= 0 || mqtt == NULL || data == NULL){
		printf("error: malloc has invallid input\n");
		return -1;
	}

	result = malloc(size);
	
	if(result == NULL){
		printf("error: malloc failed to allocate memory\n");
		*data = NULL;
		return -1;
	}

	mqtt->bytes_used += size;
	*data = result;
	return 0;
}

int my_free(struct pico_mqtt* mqtt, void* data, size_t size){
	if(size <= 0 || mqtt == NULL || data == NULL){
		printf("error: free has invallid input\n");
		return -1;
	}

	free(data);
	
	mqtt->bytes_used -= size;
	return 0;
}

int pico_mqtt_connection_send_receive( struct pico_mqtt_socket* socket, struct pico_mqtt_data* read_buffer, struct pico_mqtt_data* write_buffer, struct timeval* time_left){
	read_buffer->length -= socket->bytes_to_read;
	read_buffer->data += socket->bytes_to_read;
	write_buffer->length -= socket->bytes_to_write;
	write_buffer->data += socket->bytes_to_write;
	
	if(time_left != NULL){
		struct timeval b = {.tv_usec = socket->usecs_to_use, .tv_sec = socket->secs_to_use};
		if(timercmp(time_left, &b, <)){
			time_left->tv_sec = 0;
			time_left->tv_usec = 0;
		} else {
			timersub(time_left, &b, time_left);
		}
	}

	return socket->value_to_return;
}
