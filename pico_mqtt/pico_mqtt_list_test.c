#define _BSD_SOURCE
#define DEBUG 0
#include <stdlib.h>
#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "pico_mqtt.h"
#include "pico_mqtt_list.h"

#define ERROR -1
#define SUCCES 0

/**
* test data types
**/

struct pico_mqtt_message test_messages[10] = 
	{
		{ .message_id = 0 },
		{ .message_id = 1 },
		{ .message_id = 2 },
		{ .message_id = 3 },
		{ .message_id = 4 },
		{ .message_id = 5 },
		{ .message_id = 6 },
		{ .message_id = 7 },
		{ .message_id = 8 },
		{ .message_id = 9 },
	};

/**
* test functions prototypes
**/

void try_push(int result);
void try_peek(int result);
void try_pop(int result);
void try_length(int result);
void try_create_list(struct pico_mqtt_list** list);
void try_checked_push(struct pico_mqtt_list* list, uint32_t index, uint32_t length_after);
void try_checked_pop(struct pico_mqtt_list* list, uint32_t index, uint32_t length_after);
void try_destroy_list(struct pico_mqtt_list** list);

/**
* file under test
**/

#include "pico_mqtt_list.c"

/**
* tests
**/

START_TEST(create_and_destroy_test)
{
	struct pico_mqtt_list* list;
	int result = 0;

	result = pico_mqtt_list_create(&list);
	ck_assert_msg( result == SUCCES, "Failed to create the list\n");

	result = pico_mqtt_list_destroy(&list);
	ck_assert_msg( result == SUCCES, "Failed to destory the list\n");

}
END_TEST

START_TEST(push_peek_length_and_pop_test)
{
	struct pico_mqtt_list* list;
	struct pico_mqtt_message* message;
	int result = 0;
	uint32_t length = 0;

	result = pico_mqtt_list_create(&list);
	ck_assert_msg( result == SUCCES, "Failed to create the list\n");

	try_length(pico_mqtt_list_length(list, &length));
	ck_assert_msg( length == 0, "Failed to get the length of the list\n");

	try_push(pico_mqtt_list_push(list, &test_messages[0]));

	try_length(pico_mqtt_list_length(list, &length));
	ck_assert_msg( length == 1, "The length is not changed correctly\n");

	try_peek(pico_mqtt_list_peek(list, &message));
	ck_assert_msg( message->message_id == 0, "Failed to peek at the top of the message\n");
	message = NULL;

	try_length(pico_mqtt_list_length(list, &length));
	ck_assert_msg( length == 1, "The length should not have changed\n");

	try_pop(pico_mqtt_list_pop(list, &message));
	ck_assert_msg( message->message_id == 0, "Failed to pop the top of the message\n");

	try_length(pico_mqtt_list_length(list, &length));
	ck_assert_msg( length == 0, "The length is not changed correctly\n");

	result = pico_mqtt_list_destroy(&list);
	ck_assert_msg( result == SUCCES, "Failed to destory the list\n");

}
END_TEST

START_TEST(checked_push_and_pop_test)
{
	/* this function does exactly the same as the previous test
	to check if the functions are implemented correctly */

	struct pico_mqtt_list* list;

	try_create_list(&list);
	try_checked_push(list, 0, 1);
	try_checked_pop(list, 0, 0);
	try_destroy_list(&list);
}
END_TEST

START_TEST(multiple_push_and_pop_test)
{
	struct pico_mqtt_list* list;
	int i = 0;

	try_create_list(&list);
	for(i = 0; i<10; ++i)
	{
		try_checked_push(list, i, i+1);
	}

	for(i = 9; i>=0; --i)
	{
		try_checked_pop(list, 9-i, i);
	}

	for(i = 0; i<10; ++i)
	{
		try_checked_push(list, i, i+1);
	}

	try_destroy_list(&list);
}
END_TEST


Suite * functional_list_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("MQTT list");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, create_and_destroy_test);
	tcase_add_test(test_case_core, push_peek_length_and_pop_test);
	tcase_add_test(test_case_core, checked_push_and_pop_test);
	tcase_add_test(test_case_core, multiple_push_and_pop_test);

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

void try_push(int result)
{
	ck_assert_msg( result == SUCCES, "Failed to push a message on the queue\n");
}

void try_peek(int result)
{
	ck_assert_msg( result == SUCCES, "Failed to peek a message on the queue\n");
}

void try_pop(int result)
{
	ck_assert_msg( result == SUCCES, "Failed to pop a message on the queue\n");
}

void try_length(int result)
{
	ck_assert_msg( result == SUCCES, "Failed to read the length of the queue\n");
}
void try_create_list(struct pico_mqtt_list** list)
{
	int result = pico_mqtt_list_create(list);
	ck_assert_msg( result == SUCCES, "Failed to create the list\n");
}

void try_checked_push(struct pico_mqtt_list* list, uint32_t index, uint32_t length_after)
{
	uint32_t length = 0;

	try_length(pico_mqtt_list_length(list, &length));
	ck_assert_msg( length == length_after-1, "Failed to get the length of the list\n");

	try_push(pico_mqtt_list_push(list, &test_messages[index]));

	try_length(pico_mqtt_list_length(list, &length));
	ck_assert_msg( length == length_after, "The length is not changed correctly\n");

	try_length(pico_mqtt_list_length(list, &length));
	ck_assert_msg( length == length_after, "The length should not have changed\n");
}

void try_checked_pop(struct pico_mqtt_list* list, uint32_t index, uint32_t length_after)
{
	uint32_t length = 0;
	struct pico_mqtt_message* message = NULL;

	try_length(pico_mqtt_list_length(list, &length));
	ck_assert_msg( length == length_after+1, "The length is not changed correctly\n");

	try_pop(pico_mqtt_list_pop(list, &message));
	ck_assert_msg( message->message_id == index, "Failed to pop the top of the message\n");

	try_length(pico_mqtt_list_length(list, &length));
	ck_assert_msg( length == length_after, "The length is not changed correctly\n");
}

void try_destroy_list(struct pico_mqtt_list** list)
{
	int result = pico_mqtt_list_destroy(list);
	ck_assert_msg( result == SUCCES, "Failed to destory the list\n");
}
