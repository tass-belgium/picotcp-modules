#if 1== 0
#define DEBUG 3


#include <netdb.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <netinet/in.h>
#include <netdb.h>

#include <arpa/inet.h>

#include <stdlib.h>
#include <check.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "pico_mqtt_debug.h"
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

int tcp_v6_echo_server(struct test_thread pipes, void* data);
int echo_thread(struct test_thread pipes, void* data);
struct test_thread create_test_thread(int(*thread_function)(struct test_thread pipes, void* data), void* data);

int pico_mqtt_connection_bind(struct pico_mqtt_socket** connection, const char* uri, const char* port);
static struct addrinfo lookup_configuration_server( void );
static int resolve_uri_server( struct addrinfo** addresses, const char* uri, const char* port);
static int socket_bind( struct pico_mqtt_socket* connection, struct addrinfo* addres );


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

START_TEST(echo_server_test)
{
	struct test_thread thread;

	thread = create_test_thread( tcp_v6_echo_server, "7070" );

	close(thread.write_descriptor);
	close(thread.read_descriptor);
}
END_TEST

START_TEST(tcp_socket_create_and_destroy)
{
	int result = 0;
	struct pico_mqtt_socket* socket = NULL;
    result = pico_mqtt_connection_create(&socket);
    ck_assert_msg(result == SUCCES, "Error while allocating memory socket");

	result = pico_mqtt_connection_open(socket, "127.0.0.1", "1883");
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

    result = pico_mqtt_connection_create(&socket);
    ck_assert_msg(result == SUCCES, "Error while allocating memory socket");

	result = pico_mqtt_connection_open(socket, ip, "1883");
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

	/*tcase_add_test(test_case_core, fork_test);
	tcase_add_test(test_case_core, echo_server_test);
	tcase_add_test(test_case_core, tcp_socket_create_and_destroy);
	tcase_add_test(test_case_core, tcp_socket_send_test);*/
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
		PERROR("no thread function specified");
		exit(EXIT_FAILURE);
	}

	if(pipe(write_to_thread_fd) == -1){
		PERROR("pipe");
		exit(EXIT_FAILURE);
	}

	if(pipe(read_from_thread_fd) == -1){
		PERROR("pipe");
		exit(EXIT_FAILURE);
	}
	
	cpid = fork();

	if(cpid == -1) {
		PERROR("fork");
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

int tcp_v6_echo_server(struct test_thread pipes, void* data)
{
    int result = 0;
    char* port = (char*) data;

    int n;
    int i = 0;
    int newsockfd = 0;
    int flag = 1;
    uint8_t received_byte = 0;
    struct sockaddr_in6 client_address;
    socklen_t client_address_length;
    char client_addr_ipv6[100];
    struct pollfd poll_fd[2];


    struct pico_mqtt_socket* connection = NULL;

    PINFO("Created TCP IPv6 echo server.");

    result = pico_mqtt_connection_bind( &connection, "127.0.0.1", port);
    if(result == ERROR)
    {
        exit(-1);
    }

    result = listen(connection->descriptor, 2); /* allow only 2 connections */
    if(result != 0)
    {
        PERROR("Listen returned %d: %s\n", errno, strerror(errno));
        exit(-1);
    }

    client_address_length = sizeof(client_address);
    
    poll_fd[0] = (struct pollfd) {
        .fd = connection->descriptor,
        .events = POLLIN,
        .revents = 0
    };

    result = poll(poll_fd, 1, -1);
    if(result == -1)
    {
        PERROR("Poll returned error when waiting to accept connection (returned %d): %s\n", errno, strerror(errno));
        return ERROR;
    }

#ifdef DEBUG
    if(result == 0)
    {
        PERROR("Not possible to have a timeout!\n");
        return ERROR;
    }

    if(result > 1)
    {
        PERROR("Not possible to have more then 1 active fiel descriptor!\n");
        return ERROR;
    }
#endif

    newsockfd = accept(connection->descriptor, (struct sockaddr *) &client_address, &client_address_length);
    if (newsockfd < 0)
    {
        PERROR("ERROR on accept\n");
    }

#ifdef DEBUG
    //Sockets Layer Call: inet_ntop()
    inet_ntop(AF_INET6, &(client_address.sin6_addr),client_addr_ipv6, 100);
    printf("Incoming connection from client having IPv6 address: %s\n",client_addr_ipv6);
#endif
    
    //Sockets Layer Call: recv()
    poll_fd[0] = (struct pollfd) {
        .fd = newsockfd,
        .events = POLLIN,
        .revents = 0
    };
    poll_fd[1] = (struct pollfd) {
        .fd = pipes.read_descriptor,
        .events = POLLIN,
        .revents = 0
    };

    while(flag)
    {
        result = poll(poll_fd, 2, -1);

        #ifdef DEBUG
            if(result == 0)
            {
                PERROR("Not possible to have a timeout!\n");
                return ERROR;
            }

            if(result > 2)
            {
                PERROR("Not possible to have more then 1 active fiel descriptor!\n");
                return ERROR;
            }
        #endif

        for(i = 0; i<2; i++)
        {
        	if((poll_fd[i].revents & POLLIN) != 0)
        	{
	        	n = read(poll_fd[i].fd, &received_byte, 1);
	        
		        if (n < 0)
		        {
		            PERROR("Reading from socket returned %d: %s\n", errno, strerror(errno));
		            return ERROR;
		        }

		        if (n == 0)
		        {
		            PINFO("End of file, done reading.\n");
		            flag = 0;
		            continue;
		        }

		        if(i == 0)
		        {
		        	n = write(pipes.write_descriptor, &received_byte, 1);
		        } else {
		        	n = write(newsockfd, &received_byte, 1);
		        }

		        if(n != 1)
		        {
		        	PERROR("Unable to write byte.\n");
		        	return ERROR;
		        }
        	}

        	poll_fd[0].revents = 0;
        	poll_fd[1].revents = 0;
        }       
    }
    
    //Sockets Layer Call: close()
    close(connection->descriptor);
    close(newsockfd);
    
    return 0;
}

int pico_mqtt_connection_bind(struct pico_mqtt_socket** connection, const char* uri, const char* port)
{
    struct addrinfo* addres = NULL;
    int result = 0;
    int flags = 0;

    result = resolve_uri_server( &addres, uri, port );
    if(result == ERROR)
    {
        PERROR("Unable to resolve the uri.\n");
        return ERROR;
    }

    *connection = (struct pico_mqtt_socket*) malloc(sizeof(struct pico_mqtt_socket));
    if(connection == NULL)
    {
        PERROR("Unable to allocate memory for socket");
        return ERROR;
    }

    **connection = (struct pico_mqtt_socket) {.descriptor = 0};

    result = socket_bind( *connection, addres );

    flags = fcntl((*connection)->descriptor, F_GETFL, 0);
    fcntl((*connection)->descriptor, F_SETFL, flags | O_NONBLOCK);

    if(result == ERROR)
    {
        PTODO("check if this is the correct way to free the address info struct\n");
        PERROR("Failed to connect to a socket.\n");
        freeaddrinfo((struct addrinfo*) addres);
        PTODO("use a function to free the connection\n");
        free(*connection);
        return ERROR;
    }

    PTODO("check if this is the correct way to free the address info struct.\n");
    freeaddrinfo((struct addrinfo*) addres);

    return SUCCES;
}

/* return a list of IPv6 addresses */
static int resolve_uri_server( struct addrinfo** addresses, const char* uri, const char* port){
    const struct addrinfo hints =  lookup_configuration_server();
    int result = 0;

#ifdef DEBUG
    char addres_string[100];
#endif

#ifdef DEBUG
    if((addresses == NULL) || (port == NULL))
    {
        PERROR("Invallid arguments (%p, %p)\n", addresses, port);
        return ERROR;
    }
#endif

    result = getaddrinfo( uri, port, &hints, addresses );
    if(result != 0){
        PERROR("getaddrinfo returned %d for URI %s: %s\n", result, uri, gai_strerror(result)); 
        return ERROR;
    }

#ifdef DEBUG
    inet_ntop ((*addresses)->ai_family, &((struct sockaddr_in6*)((*addresses)->ai_addr))->sin6_addr ,addres_string, 100);
    PINFO("resolved URI (%s) to address: %s\n", uri, addres_string);
#endif

    return SUCCES;
}

/* return the socket file descriptor */
static int socket_bind(struct pico_mqtt_socket* connection, struct addrinfo* addres ){
    struct addrinfo *current_addres = NULL;
    int result = 0;
#ifdef DEBUG
    if((connection == NULL) || (addres == NULL))
    {
        return ERROR;
    }
#endif

    PTODO("Set the socket descriptor to non blocking.\n");

    for (current_addres = addres; current_addres != NULL; current_addres = current_addres->ai_next) {
        connection->descriptor = socket(current_addres->ai_family, current_addres->ai_socktype, current_addres->ai_protocol);
    
        if(connection->descriptor == -1)
        {
            PINFO("Failed to create a socket.\n");
            continue;
        }
        
        result = bind(connection->descriptor, current_addres->ai_addr, current_addres->ai_addrlen);
        if( result == ERROR )
        {
            PINFO("One of the addresses was not suitable to bind\n");
            close(connection->descriptor);
            continue;
        } else
        {
            PINFO("Succesfully bound to a socket\n");
            return SUCCES;
        }
        
    }
    
    PINFO("Unable to bind to any of the addresses.\n");
    return ERROR;
}


/* return the configuration for the IP lookup*/
static struct addrinfo lookup_configuration_server( void )
{
    struct addrinfo hints;
    int* flags = &(hints.ai_flags);


    /* configure the hints struct */
    hints.ai_family = AF_INET6; /* allow only IPv6 address, IPv4 will be mapped to IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* only use TCP sockets */
    hints.ai_protocol = 0; /*allow any protocol //TODO check for restrictions */
    
    /* clear the flags before setting them*/
    *flags = 0;

    /* set the appropriate flags*/
    /**flags |= AI_NUMERICHOST; unuset, lengthy network setups are allowed.*/
    /**flags |= AI_PASSIVE; can be set, no addess set for the server*/
    /* AI_NUMERICSERV unsed, //TODO check for this restriction*/
    /*AI_ADDRCONFIG unsed, intedded to use IPv6 of mapped IPv4 addresses*/
    *flags |= AI_V4MAPPED; /* map IPv4 addresses to IPv6 addresses*/
    *flags |= AI_ALL; /* return both IPv4 and IPv6 addresses*/

    /* set the unused variables to 0 */
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;

    return hints;
}




#endif




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

#include "pico_mqtt_socket.c"

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