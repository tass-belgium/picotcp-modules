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

Suite * mqtt_test_suite(void);

/**
* file under test
**/

#include "pico_mqtt.c"

/**
* tests
**/

START_TEST(dummy_test)
{


	CHECK_NO_ALLOCATIONS();
}
END_TEST



Suite * mqtt_test_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("MQTT serializer");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, dummy_test);

	suite_add_tcase(test_suite, test_case_core);

	return test_suite;
}

 int main(void)
 {
	int number_failed;
	Suite *test_suite;
	SRunner *suite_runner;

	test_suite = mqtt_test_suite();
	suite_runner = srunner_create(test_suite);

	srunner_run_all(suite_runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(suite_runner);
	srunner_free(suite_runner);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }