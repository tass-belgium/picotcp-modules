#include <stdlib.h>
#include <check.h>
#include <sys/socket.h>
#include "../../pico_mqtt_socket.h"
#include <stdio.h>

START_TEST(tcp_socket_create)
{
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	ck_assert_msg(socketfd != -1, "Error while creating the socket");
}
END_TEST

Suite * tcp_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("TCP");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, tcp_socket_create);
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
