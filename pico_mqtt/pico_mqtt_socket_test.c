#include <stdlib.h>
#include <check.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "pico_mqtt_socket.h"
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/**
* test data types
**/

struct test_thread{
	int read_descriptor;
	int write_descriptor;
};

/**
* test functions prototypes
**/

int open_local_socket_v4();
int echo_thread(struct test_thread pipes, void* data);
struct test_thread create_test_thread(int(*thread_function)(struct test_thread pipes, void* data), void* data);

/**
* tests
**/

START_TEST(fork_test)
{
	int result = 0;
	struct test_thread thread;
	char* buffer = (char*) malloc(1);
	*buffer = 'a';
	thread = create_test_thread(echo_thread, (void*)1);
	result = write(thread.write_descriptor, buffer, 1);
	*buffer = 'b';
	result = read(thread.read_descriptor, buffer, 1);
	if(result == -1)
		printf("error: %s\n", strerror(errno));
	close(thread.write_descriptor);
	close(thread.read_descriptor);
	wait(NULL);
	ck_assert_msg(*buffer == 'a', "thread not writing to parent");
}
END_TEST

START_TEST(tcp_socket_create_and_destroy)
{
	int error = 0;
	struct pico_mqtt_socket* dummy = pico_mqtt_connection_open( "127.0.0.1", 0);
	ck_assert_msg(dummy != NULL, "Error while creating the socket");
	error = pico_mqtt_connection_close( dummy );
	ck_assert_msg(error == 0, "Error while closing the socket");
}
END_TEST

Suite * tcp_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("TCP");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, fork_test);
	tcase_add_test(test_case_core, tcp_socket_create_and_destroy);
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


/* fork the process, set up 2 pipes to the new process */
struct test_thread create_test_thread(int(*thread_function)(struct test_thread pipes, void* data), void* data){
	struct test_thread thread = {.read_descriptor = 0, .write_descriptor = 0};
	int write_to_thread_fd[2];
	int read_from_thread_fd[2];
	pid_t cpid;

	if(thread_function == NULL){
		perror("no thread function specified");
		exit(EXIT_FAILURE);
	}

	if(pipe(write_to_thread_fd) == -1){
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	if(pipe(read_from_thread_fd) == -1){
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	
	cpid = fork();

	if(cpid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if(cpid == 0){ /* child process */
		int result;
		close(write_to_thread_fd[1]); /* close write end */
		close(read_from_thread_fd[0]); /* close read end */
		thread.read_descriptor = write_to_thread_fd[0];
		thread.write_descriptor = read_from_thread_fd[1];
		result = thread_function( thread, data);
		close(write_to_thread_fd[0]);
		close(read_from_thread_fd[1]);
		exit(result);
	} else {
		close(write_to_thread_fd[0]); /* close read end */
		close(read_from_thread_fd[1]); /* close write end */
		thread.read_descriptor = read_from_thread_fd[0];
		thread.write_descriptor = write_to_thread_fd[1];
		return thread;
	}
}

int echo_thread(struct test_thread pipes, void* data){
	char buffer[1];
	int bytes_red;
	int* dummy = (int*) data;
	dummy++;
	buffer[0] = 'b';
	bytes_red = read(pipes.read_descriptor, buffer, 1);
	if(bytes_red <= 0)
		exit(1);

	ck_assert_msg(*buffer == 'a', "thread not reading from parent");
	write(pipes.write_descriptor, buffer, 1);
	close(pipes.read_descriptor);
	close(pipes.write_descriptor);
	return 0;
}

/* open tcp socket @127.0.0.1:1883 */
int open_local_socket_v4(){
	int fd;
	struct sockaddr_in server_addres;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	server_addres.sin_family = AF_INET;
	server_addres.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addres.sin_port=htons(1883);
	
	bind(fd,(struct sockaddr*)&server_addres, sizeof(server_addres));

	listen(fd, 1);

	return 0;
}
