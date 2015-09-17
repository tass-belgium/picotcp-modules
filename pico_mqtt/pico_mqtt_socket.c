#include "pico_mqtt_error.h"
#include "pico_mqtt_socket.h"

/**
* data structures
**/

struct pico_mqtt_socket{
	int descriptor;
};

/**
* private functions prototypes
**/

static struct pico_mqtt_socket* connection_open( const char* URI );
static int connection_read(struct pico_mqtt_socket* socket, void* read_buffer, const uint32_t count);
static int resolve_URI( struct addrinfo** addresses, const char* URI);
static int socket_connect( struct pico_mqtt_socket* socket, struct addrinfo* addresses );
static struct addrinfo lookup_configuration( void );

/**
* public functions implementations
**/

/* create pico mqtt socket and connect to the URI, returns NULL on error*/
int pico_mqtt_connection_open(struct pico_mqtt_socket** socket, const char* URI, struct timeval* time_left)
{
	struct addrinfo* addres = NULL;
	int result = 0;

#ifdef DEBUG
	if(socket == NULL)
	{
		PERROR("invallid arguments");
		return ERROR;
	}
#endif

	result = resolve_URI( &addres, URI );
	if(result == ERROR)
	{
		return ERROR;
	}

	*socket = (struct pico_mqtt_socket*) malloc(sizeof(struct pico_mqtt_socket));

	result = socket_connect( *socket, addres );
	if(result == ERROR)
	{
		PTODO("check if this is the correct way to free the address info struct"); /* //TODO */
		freeaddrinfo((struct addrinfo*) addres);
		free(*socket);
		return ERROR;
	}

	PTODO("check if this is the correct way to free the address info struct"); /* //TODO */
	freeaddrinfo((struct addrinfo*) addres);

	return SUCCES;
}

/* read data from the socket, return the number of bytes read */
int pico_mqtt_connection_read( struct pico_mqtt_socket* socket, struct pico_mqtt_data* read_buffer, struct timeval* time_left)
{
	PTODO("refactor this code"); /* //TODO */
	struct pollfd poll_options;
	int result;
	
	if(socket == NULL){
		return -1; /*//TODO set errno */
	}

	poll_options.fd = socket->descriptor;
	poll_options.events = 0;
	poll_options.events |= POLLIN; /* wait until their is data to read*/
	poll_options.revents = 0;

/*	result = poll(&poll_options, 1, timeout);
	if(result < 0){  error */
/*		return -1;
	}

	if(result == 0){  timeout */
/*		return 0;
	}

	if(result > 1){  implementation error, only 1 fd was passed*/
/*		return -1;
	}

	result =  connection_read(socket, read_buffer, count);
	return result;*/
	return SUCCES;
}

/* write data to the socket, return the number of bytes written*/
int pico_mqtt_connection_send_receive( struct pico_mqtt_socket* connection, struct pico_mqtt_data* read_buffer, struct pico_mqtt_data* write_buffer, struct timeval* time_left)
{
	PTODO("Set the file descriptors for exeptions\n");
	fd_set write_file_descriptors;
/*	fd_set exeption_file_descriptors;*/
	int result = 0;
#ifdef DEBUG
	if((connection == NULL) || (write_buffer == NULL) || (time_left == NULL))
	{
		PERROR("invallid arugments\n");
		return ERROR;
	}
#endif
	
	FD_ZERO(&write_file_descriptors);
/*	FD_ZERO(&exeption_file_descriptors);*/
	FD_SET(connection->descriptor, &write_file_descriptors);
/*	FD_SET(connection->descriptor, &exeption_file_descriptors);*/

	result = select(2, NULL, &write_file_descriptors, NULL, time_left);
/*	result = select(2, NULL, &write_file_descriptor, &exeption_file_descriptors, time_left);*/

	if(result == -1)
	{
		PERROR("Select returned an error: %d - \n", errno, strerror(errno));
		return ERROR;
	}

	if(result == 0)
	{
		return SUCCES; /* a timeout occurred */
	}

#ifdef DEBUG
	if(result > 1)
	{
		PERROR("It should not be possible to have more then 1 active file descriptor.\n");
	}

	if(!FD_ISSET(connection->descriptor, &write_file_descriptors))
	{
		PERROR("Select returned without error, a connections should be ready to read\n");
		return ERROR;
	}
#endif

	if(result == 1)
	{
		uint32_t bytes_written = 0;
		bytes_written = write(connection->descriptor, write_buffer->data, write_buffer->length);
		write_buffer->data += bytes_written;
		write_buffer->length -= bytes_written;
	}

	return SUCCES;	
}

/* close pico mqtt socket, return -1 on error */
int pico_mqtt_connection_close( struct pico_mqtt_socket** socket)
{
	int result = 0;
#ifdef DEBUG
	if(socket == NULL)
	{
		PERROR("invallid arguments\n");
		return ERROR;
	}
#endif
	result =  close((*socket)->descriptor);
	if( result == ERROR)
	{
		return ERROR;
	}

	free(*socket);
	*socket = NULL;

	return SUCCES;
}

/**
* private function implementation
**/
/*
static struct pico_mqtt_socket* connection_open( const char* URI ){
}
*/
/* return the socket file descriptor */
static int socket_connect( struct pico_mqtt_socket* connection, struct addrinfo* addres ){
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
			continue;
		
		result = connect(connection->descriptor, current_addres->ai_addr, current_addres->ai_addrlen);
		if( result == ERROR )
		{
			close(connection->descriptor);
			continue;
		} else
		{
			return SUCCES;
		}
		
	}
	
	return ERROR;
}

/* return a list of IPv6 addresses */
static int resolve_URI( struct addrinfo** addresses, const char* URI ){
	const struct addrinfo hints =  lookup_configuration();
	int result = 0;

#ifdef DEBUG
	char addres_string[100];
#endif

#ifdef DEBUG
	if( addresses == NULL )
	{
		PERROR("invallid arguments");
		return ERROR;
	}
#endif

	/* //TODO specify the service, no hardcoded ports.*/
	result = getaddrinfo( URI, "1883", &hints, addresses );
	if(result != 0){
		printf("error %d: %s\n", errno, strerror(errno)); 
		return ERROR;
	}
#ifdef DEBUG
	inet_ntop ((*addresses)->ai_family, &((struct sockaddr_in6*)((*addresses)->ai_addr))->sin6_addr ,addres_string, 100);
	PINFO("resolved URI(%s) to address: %s\n", URI, addres_string);
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
#if (PICO_MQTT_DNS_LOOKUP == 1)
	*flags |= AI_NUMERICHOST;
#endif /*(PICO_MQTT_DNS_LOOKUP = 1)*/
	/* AI_PASSIVE unsed, intend to use address for a connect*/
	/* AI_NUMERICSERV unsed, //TODO check for this restriction*/
#if (PICO_MQTT_HOSTNAME_LOOKUP ==1)
	*flags |= IA_CANONNAME;
#endif /*(PICO_MQTT_HOSTNAME_LOOKUP ==1)*/
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

static int connection_read(struct pico_mqtt_socket* socket, void* read_buffer, const uint32_t count){
	return read(socket->descriptor, read_buffer, count);
}
