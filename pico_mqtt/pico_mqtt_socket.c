
#include "pico_mqtt_socket.h"

/**
* data structures
**/

struct pico_mqtt_socket{
	int descriptor;
	int* error;
};

#define PICO_MQTT_SOCKET_EMPTY (struct pico_mqtt_socket){\
	.descriptor = 0,\
	.error = NULL\
}

/**
* Private function prototypes
**/

static struct addrinfo lookup_configuration( void );
static int resolve_uri( struct pico_mqtt_socket* connection, struct addrinfo** addresses, const char* uri, const char* port);
static int socket_connect( struct pico_mqtt_socket* connection, struct addrinfo* addres);

/**
* Public function implementation
**/

/* create pico mqtt socket */
struct pico_mqtt_socket* pico_mqtt_connection_create( int* error )
{
	struct pico_mqtt_socket* socket = NULL;

	CHECK_NOT_NULL(error);

	socket = (struct pico_mqtt_socket*) MALLOC(sizeof(struct pico_mqtt_socket));

	if(socket == NULL)
		return NULL;

	*socket =PICO_MQTT_SOCKET_EMPTY;
	socket->error = error;

	return socket;
}

int pico_mqtt_connection_open(struct pico_mqtt_socket* connection, const char* URI, const char* port)
{
	struct addrinfo* addres = NULL;
	int flags = 0;

	CHECK_NOT_NULL(connection);

	if(resolve_uri(connection, &addres, URI, port) == ERROR)
		return ERROR;

	if(socket_connect(connection, addres) == ERROR)
	{
		freeaddrinfo(addres);
		return ERROR;
	}

	flags = fcntl(connection->descriptor, F_GETFL, 0);
	if(fcntl(connection->descriptor, F_SETFL, flags | O_NONBLOCK) != 0)
	{
		freeaddrinfo(addres);
		PERROR("fnctl was not able to set the socket to Nonblocking mode error (%d): %s.\n", errno, strerror(errno));
		PTODO("set the appropriate error.\n");
		return ERROR;
	}

	freeaddrinfo(addres);

	return SUCCES;
}

int pico_mqtt_connection_send_receive( struct pico_mqtt_socket* connection, struct pico_mqtt_data* write_buffer, struct pico_mqtt_data* read_buffer, uint64_t time_left)
{
/*	int result = 0;
	struct pollfd poll_descriptor = (struct pollfd) {
		.fd = connection->descriptor,
		.events = 0,
		.revents = 0
	};*/
		time_left++;
	CHECK_NOT_NULL(connection);
	CHECK_NOT_NULL(write_buffer);
	CHECK_NOT_NULL(read_buffer);
	CHECK(((fcntl(connection->descriptor, F_GETFL, 0) & O_NONBLOCK) != 0),
		"Nonblocking flags should be set for the socket.\n");

/*	if((write_buffer->length == 0) && (read_buffer->length == 0))
	{
		PWARNING("Called send and receive function without data to be send or received.\n");
		return SUCCES;
	}

	if(write_buffer->length != 0)
	{
		poll_descriptor.revents |= POLLOUT;
	}

	if(read_buffer->length != 0)
	{
		poll_descriptor.revents |= POLLIN;
	}

	result = poll(&poll_descriptor, 1, (int32_t)time_left);

	CHECK((result < 2), "It should not be possible to have more then 1 active file descriptor.\n");

	if(result == -1)
	{
		PERROR("Poll returned an error (%d): %s\n", errno, strerror(errno));
		return ERROR;
	}

	if(result == 0)
	{
		PINFO("A timeout occurred while sending and receiving.\n");
		return SUCCES;
	}

	if(result == 0)
	{
		CHECK(((poll_descriptor.revents & (POLLIN | POLLOUT)) == 0),
			"Poll returned without error, data should be ready to write or read but is not.\n");
*/
//		if(((poll_descriptor.revents & POLLIN) != 0) && (read_buffer->length != 0)) /* data to read */
		if((read_buffer->length != 0)) /* data to read */
		{
			int64_t bytes_written = 0;
			bytes_written = read(connection->descriptor, read_buffer->data, read_buffer->length);

			if(bytes_written < 0)
			{
				if(errno != 11)
					PERROR("read returned an error (%d): %s\n", errno, strerror(errno));
			} else
			{
				read_buffer->data += (uint32_t) bytes_written;
				read_buffer->length -= (uint32_t) bytes_written;
				if(bytes_written != 0)
					PINFO("Read %d bytes from %d\n", (uint32_t) bytes_written, connection->descriptor);
			}
		}

//		if(((poll_descriptor.revents & POLLOUT) != 0) && (write_buffer->length != 0))/* data to write */
		if((write_buffer->length != 0))/* data to write */
		{
			int64_t bytes_written = 0;
			bytes_written = write(connection->descriptor, write_buffer->data, write_buffer->length);
			
			if(bytes_written < 0)
			{
				PERROR("write returned an error (%d): %s\n", errno, strerror(errno));
			} else
			{
				write_buffer->data += (uint32_t) bytes_written;
				write_buffer->length -= (uint32_t) bytes_written;
				PINFO("Written %d bytes to %d\n", (uint32_t) bytes_written, connection->descriptor);
			}
		}/*
	} else {
		PERROR("Poll returned %d, only expected values are -1, 0 or 1.\n", result);
		PTODO("Set a specific error.\n");
		return ERROR;
	}*/

	return SUCCES;
}

void pico_mqtt_connection_destroy( struct pico_mqtt_socket* socket)
{
	if(socket == NULL)
		return;

	close(socket->descriptor);

	FREE(socket);
}

uint64_t get_current_time( void  )
{
	struct timeval now;
	gettimeofday(&now, NULL);
	return ((uint64_t)now.tv_sec * 1000) + ((uint64_t)now.tv_usec / 1000);
}

/**
* Private function implementation
**/

/* return a list of IPv6 addresses */
static int resolve_uri( struct pico_mqtt_socket* connection, struct addrinfo** addresses, const char* uri, const char* port){
	const struct addrinfo hints =  lookup_configuration();
	int result = 0;
#ifdef DEBUG
	char addres_string[100];
#endif

	CHECK_NOT_NULL(addresses);
	CHECK_NOT_NULL(port);

	result = getaddrinfo( uri, port, &hints, addresses );
	if(result != 0){
		PERROR("getaddrinfo returned %d for URI %s: %s\n", result, uri, gai_strerror(result));
		*connection->error = URI_LOOKUP_FAILED;
		return ERROR;
	}

#ifdef DEBUG
	inet_ntop ((*addresses)->ai_family, &((struct sockaddr_in6*)((*addresses)->ai_addr))->sin6_addr ,addres_string, 100);
	PINFO("resolved URI (%s) to address: %s\n", uri, addres_string);
#endif

	return SUCCES;
}

/* return the configuration for the IP lookup*/
static struct addrinfo lookup_configuration( void ){
	struct addrinfo hints;
	int* flags = &(hints.ai_flags);


	/* configure the hints struct */
	hints.ai_family = AF_INET6; /* allow only IPv6 address, IPv4 will be mapped to IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* only use TCP sockets */
	hints.ai_protocol = 0; /*allow any protocol //TODO check for restrictions */

	/* clear the flags before setting them*/
	*flags = 0;

	/* set the appropriate flags*/
#if (ENBABLE_DNS_LOOKUP == 0)
	*flags |= AI_NUMERICHOST;
#endif /*(ENBABLE_DNS_LOOKUP == 0)*/
	/* AI_PASSIVE unset, intend to use address for a connect*/
	/* AI_NUMERICSERV unset, //TODO check for this restriction*/
	/* IA_CANONNAME unset, don't care about the official name of the server*/
	/* AI_ADDRCONFIG unset, intend to use IPv6 of mapped IPv4 addresses*/
	*flags |= AI_V4MAPPED; /* map IPv4 addresses to IPv6 addresses*/
	*flags |= AI_ALL; /* return both IPv4 and IPv6 addresses*/

	/* set the unused variables to 0 */
	hints.ai_addrlen = 0;
	hints.ai_addr = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;

	return hints;
}

/* return the socket file descriptor */
static int socket_connect( struct pico_mqtt_socket* connection, struct addrinfo* addres){
	struct addrinfo *current_addres = NULL;

	CHECK_NOT_NULL(connection);
	CHECK_NOT_NULL(addres);

	PTODO("Set the socket descriptor to non blocking.\n");

	for (current_addres = addres; current_addres != NULL; current_addres = current_addres->ai_next) {
		connection->descriptor = socket(current_addres->ai_family, current_addres->ai_socktype, current_addres->ai_protocol);

		if(connection->descriptor == -1)
		{
			PINFO("Failed to create a socket.\n");
			continue;
		}

		if(connect(connection->descriptor, current_addres->ai_addr, current_addres->ai_addrlen) == ERROR)
		{
			PINFO("One of the addresses was not suitable for a connection\n");
			close(connection->descriptor);
			continue;
		} else
		{
			PINFO("Succesfully connected to a socket\n");
			return SUCCES;
		}

	}

	PERROR("Unable to connect to any of the addresses.\n");
	return ERROR;
}
