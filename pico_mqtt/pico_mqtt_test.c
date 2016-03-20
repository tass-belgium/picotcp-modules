#define _BSD_SOURCE

#include <check.h>
#include <stdlib.h>

#define DEBUG 3

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

#include "pico_mqtt.c"

/**
* tests
**/

START_TEST(pico_mqtt_list_create_test)
{
	int error = 0;
	struct pico_mqtt_list* list = NULL;

	MALLOC_SUCCEED();
	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed\n");
	ck_assert_msg(list->first == NULL, "The first elements should be NULL.\n");
	ck_assert_msg(list->last == NULL, "The last elements should be NULL.\n");
	FREE(list);

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list == NULL, "Creating the list should have failed\n");;

	CHECK_NO_ALLOCATIONS();
}
END_TEST

Suite * functional_list_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("MQTT serializer");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, compare_arrays_test);

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