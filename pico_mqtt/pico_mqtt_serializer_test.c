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
};

/**
* test functions prototypes
**/

int my_malloc(struct pico_mqtt* mqtt, void** data, size_t size);
int my_free(struct pico_mqtt* mqtt, void* data, size_t size);

/**
* file under test
**/

#include "pico_mqtt_serializer.c"

/**
* tests
**/

START_TEST(create_and_destroy_test)
{
	int result = 0;
	struct pico_mqtt_serializer* serializer;
	struct pico_mqtt mqtt = {.bytes_used = 0};

	result = pico_mqtt_serializer_create( &mqtt, &serializer, my_malloc, my_free);

	ck_assert_msg( result == 0, "Failed to create the serializer\n");

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

	tcase_add_test(test_case_core, create_and_destroy_test);

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
