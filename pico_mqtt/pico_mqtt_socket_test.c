#define DEBUG 3

#include <stdlib.h>
#include <check.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "pico_mqtt_error.h"
#include "pico_mqtt_socket.h"
#include "pico_mqtt_socket.c"

/**
* test data types
**/

struct test_thread
{
	int read_descriptor;
	int write_descriptor;
};

/**
* test functions prototypes
**/

int open_local_socket_v4();
int tcp_v4_echo_server(struct test_thread pipes, void* data);
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
	int result = 0;
	struct pico_mqtt_socket* socket = NULL;
	result = pico_mqtt_connection_open(&socket, "127.0.0.1", "1883");
	ck_assert_msg(socket != NULL, "Error while creating the socket");
	ck_assert_msg(result == SUCCES, "Error while creating the socket");
	result = pico_mqtt_connection_close( &socket );
	ck_assert_msg(result == SUCCES, "Error while closing the socket");
}
END_TEST

START_TEST(tcp_socket_send_test)
{
	struct pico_mqtt_socket* socket;
	int result = 0;
	struct timeval time_left = {.tv_usec = 0, .tv_sec = 1};
	/*struct test_thread thread;*/
	char buffer[100] = "Hallo world";
	struct pico_mqtt_data data = {.length = 12, .data = buffer};

	char* ip = "127.0.0.1";
	/*thread = create_test_thread(tcp_v4_echo_server, (void*) NULL);*/
	/*sleep(1);*/
	result = pico_mqtt_connection_open(&socket, ip, "1883");
	ck_assert_msg(result == SUCCES, "Unable to find host");

	/**buffer = 'a';*/
	result = pico_mqtt_connection_send(socket, &data, &time_left);
	ck_assert_msg(result == SUCCES, "Error while writing to the socket.");
/*
	*buffer = 'b';
	result = read(thread.read_descriptor, buffer, 1);
	if(result == -1)
		printf("error: %s\n", strerror(errno));
	close(thread.write_descriptor);
	close(thread.read_descriptor);
	wait(NULL);
	ck_assert_msg(*buffer == 'a', "thread not writing to parent");*/
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
	tcase_add_test(test_case_core, tcp_socket_send_test);
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
	printf("create tcp socket\n");
	int fd;
	struct sockaddr_in server_addres;
	struct sockaddr_in client_addres;
	int connection_fd;
	int result = 0;
	socklen_t client_length;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1){
		printf("error: %s\n", strerror(errno));
		exit(1);
	}
	server_addres.sin_family = AF_INET;
	server_addres.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	server_addres.sin_port=htons(1883);
	
	printf("socket created\n");
	bind(fd,(struct sockaddr*)&server_addres, sizeof(server_addres));
	if(result == -1){
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	printf("bound \n");
	result = listen(fd, 1);
	if(result == -1){
		printf("error: %s\n", strerror(errno));
		exit(1);
	}
	
	printf("listening\n");
	client_length = sizeof(client_addres);
	connection_fd = accept(fd, (struct sockaddr *)&client_addres, &client_length );
	if(connection_fd == -1){
		printf("error: %s\n", strerror(errno));
		exit(1);
	}
	printf("connection accepted\n");

	return connection_fd;
}

int tcp_v4_echo_server(struct test_thread pipes, void* data){
	printf("tcp_echo_server_open\n");
	uint64_t direction = (uint64_t) data;
	int bytes_red;
	char* buffer = (char*)malloc(1);
	*buffer = 'b';

	int fd = open_local_socket_v4();
	printf("socket open\n");
	if(fd == -1){
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	if(direction == 0){ /*receive from socket*/
		bytes_red = read(pipes.read_descriptor, buffer, 1);
		if(bytes_red == -1){
			printf("error: %s\n", strerror(errno));
			exit(1);
		}
	} else { /* sending to socket */
	}
	return 0;
}
