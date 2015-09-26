#include "pico_mqtt_error.h"
#include "pico_mqtt_socket.h"

/**
* data structures
**/

struct pico_mqtt_socket{
	int descriptor;
};

/**
* Private function prototypes
**/

static int allocate_socket( struct pico_mqtt_socket** socket);
static struct addrinfo lookup_configuration( void );
static int resolve_uri( struct addrinfo** addresses, const char* uri, const char* port);
static int socket_connect( struct pico_mqtt_socket* connection, struct addrinfo* addres );

/**
* private functions prototypes
**/


/* create pico mqtt socket and connect to the URI*/ 
int pico_mqtt_connection_open(struct pico_mqtt_socket** connection, const char* URI, const char* port)
{
	struct addrinfo* addres = NULL;
	int result = 0;
	int flags = 0;

#ifdef DEBUG
	if(connection == NULL)
	{
		PERROR("Invallid arguments (%p)\n", connection);
		return ERROR;
	}
#endif

	result = resolve_uri( &addres, URI, port );
	if(result == ERROR)
	{
		return ERROR;
	}

	result = allocate_socket(connection);
	if(result == ERROR)
	{
		return ERROR;
	}

	result = socket_connect( *connection, addres );

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

/* read data from the socket, add this to the read buffer */ 
int pico_mqtt_connection_receive( struct pico_mqtt_socket* connection, struct pico_mqtt_data* read_buffer, struct timeval* time_left)
{
	PTODO("Set the file descriptors for exeptions\n");
	fd_set read_file_descriptors;
/*	fd_set exeption_file_descriptors;*/
	int result = 0;
#ifdef DEBUG
	int flags = 0;

	if((connection == NULL) || (read_buffer == NULL) || (time_left == NULL))
	{
		PERROR("invallid arugments (%p, %p, %p)\n", connection, read_buffer, time_left);
		return ERROR;
	}

	flags = fcntl(connection->descriptor, F_GETFL, 0);
	if(( flags & O_NONBLOCK) == 0)
	{
		PERROR("Nonblocking flags should be set for the socket.\n");
	}
#endif
	
	FD_ZERO(&read_file_descriptors);
/*	FD_ZERO(&exeption_file_descriptors);*/
	FD_SET(connection->descriptor, &read_file_descriptors);
/*	FD_SET(connection->descriptor, &exeption_file_descriptors);*/

	result = select(2, &read_file_descriptors, NULL, NULL, time_left);
/*	result = select(2, NULL, &write_file_descriptor, &exeption_file_descriptors, time_left);*/
	PTODO("Use the result of select, uncomment the debug check and remove the overwrite value below.\n");
	result = 1;

	if(result == -1)
	{
		PERROR("Select returned an error (%d) %s - \n", errno, strerror(errno));
		return ERROR;
	}

	if(result == 0)
	{
		PINFO("A timeout occurred for select.\n");
		return SUCCES;
	}

#ifdef DEBUG
	if(result > 1)
	{
		PERROR("It should not be possible to have more then 1 active file descriptor.\n");
	}

/*	if(!FD_ISSET(connection->descriptor, &read_file_descriptors))
	{
		PERROR("Select returned without error, a connections should be ready to read\n");
		return ERROR;
	}*/
#endif

	if(result == 1)
	{
		uint32_t bytes_written = 0;
		bytes_written = read(connection->descriptor, read_buffer->data, read_buffer->length);
		read_buffer->data += bytes_written;
		read_buffer->length -= bytes_written;
		PINFO("Written %d bytes to %d\n", bytes_written, connection->descriptor);
	}

	return SUCCES;
}

/* write data to the socket, remove this from the write buffer*/ 
int pico_mqtt_connection_send( struct pico_mqtt_socket* connection, struct pico_mqtt_data* write_buffer, struct timeval* time_left)
{
	PTODO("Set the file descriptors for exeptions\n");
	fd_set write_file_descriptors;
/*	fd_set exeption_file_descriptors;*/
	int result = 0;
#ifdef DEBUG
	int flags = 0;

	if((connection == NULL) || (write_buffer == NULL) || (time_left == NULL))
	{
		PERROR("invallid arugments (%p, %p, %p)\n", connection, write_buffer, time_left);
		return ERROR;
	}

	flags = fcntl(connection->descriptor, F_GETFL, 0);
	if(( flags & O_NONBLOCK) == 0)
	{
		PERROR("Nonblocking flags should be set for the socket.\n");
	}
#endif
	
	FD_ZERO(&write_file_descriptors);
/*	FD_ZERO(&exeption_file_descriptors);*/
	FD_SET(connection->descriptor, &write_file_descriptors);
/*	FD_SET(connection->descriptor, &exeption_file_descriptors);*/

	PTODO("Make vallid again!\n");
	result = select(/*connection->descriptor +*/ 1, NULL, &write_file_descriptors, NULL, time_left);
/*	result = select(2, NULL, &write_file_descriptor, &exeption_file_descriptors, time_left);*/
	PTODO("Use the result of select, uncomment the debug check and remove the overwrite value below.\n");
	result = 1;

	if(result == -1)
	{
		PERROR("Select returned an error (%d) %s - \n", errno, strerror(errno));
		return ERROR;
	}

	if(result == 0)
	{
		PINFO("A timeout occurred for select.\n");
		return SUCCES;
	}

#ifdef DEBUG
	if(result > 1)
	{
		PERROR("It should not be possible to have more then 1 active file descriptor.\n");
	}

/*	if(!FD_ISSET(connection->descriptor, &write_file_descriptors))
	{
		PERROR("Select returned without error, a connections should be ready to read\n");
		return ERROR;
	}*/
#endif

	if(result == 1)
	{
		uint32_t bytes_written = 0;
		bytes_written = write(connection->descriptor, write_buffer->data, write_buffer->length);
		write_buffer->data += bytes_written;
		write_buffer->length -= bytes_written;
		PINFO("Written %d bytes to %d\n", bytes_written, connection->descriptor);
	}

	return SUCCES;
}

int pico_mqtt_connection_send_receive( struct pico_mqtt_socket* connection, struct pico_mqtt_data* write_buffer, struct pico_mqtt_data* read_buffer, struct timeval* time_left)
{
	PTODO("Write implementation.\n");
	connection++;
	write_buffer++;
	read_buffer++;
	time_left++;
	return SUCCES;
}

/* close pico mqtt socket*/ 
int pico_mqtt_connection_close( struct pico_mqtt_socket** socket)
{
	PTODO("Write implementation.\n");
	socket++;
	return SUCCES;
}

/**
* Private function implementation
**/

static int allocate_socket( struct pico_mqtt_socket** connection)
{
#ifdef DEBUG
	if(connection == NULL)
	{
		PERROR("Invallid arguments (%p)\n", connection);
		return ERROR;
	}
#endif

	*connection = (struct pico_mqtt_socket*) malloc(sizeof(struct pico_mqtt_socket));
	**connection = (struct pico_mqtt_socket) {.descriptor = 0};

	return SUCCES;
}

/* return a list of IPv6 addresses */
static int resolve_uri( struct addrinfo** addresses, const char* uri, const char* port){
	const struct addrinfo hints =  lookup_configuration();
	int result = 0;

#ifdef DEBUG
	char addres_string[100];
#endif

#ifdef DEBUG
	if( addresses == NULL )
	{
		PERROR("Invallid arguments (%p)\n", addresses);
		return ERROR;
	}
#endif

	/* //TODO specify the service, no hardcoded ports.*/
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
		{
			PINFO("Failed to create a socket.\n");
			continue;
		}
		
		result = connect(connection->descriptor, current_addres->ai_addr, current_addres->ai_addrlen);
		if( result == ERROR )
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
	
	PINFO("Unable to connect to any of the addresses.\n");
	return ERROR;
}