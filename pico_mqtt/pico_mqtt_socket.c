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
static const struct addrinfo* resolve_URI( const char* URI );
static int socket_connect( const struct addrinfo* addresses );
static struct addrinfo lookup_configuration( void );

/**
* public functions implementations
**/

/* create pico mqtt socket and connect to the URI, returns NULL on error*/
struct pico_mqtt_socket* pico_mqtt_connection_open( const char* URI, int timeout){
	uint32_t temp = timeout;
	temp++;
	return connection_open( URI );
}

/* read data from the socket, return the number of bytes read */
int pico_mqtt_connection_read( struct pico_mqtt_socket* socket, void* read_buffer, const uint32_t count, int timeout){
	struct pollfd poll_options;
	int result;
	
	if(socket == NULL){
		return -1; /*//TODO set errno */
	}

	poll_options.fd = socket->descriptor;
	poll_options.events = 0;
	poll_options.events |= POLLIN; /* wait until their is data to read*/
	poll_options.events |= POLLPRI; /* wait until their is urgent data to read*/
	poll_options.revents = 0;

	result = poll(&poll_options, 1, timeout);
	if(result < 0){ /* error */
		return -1;
	}

	if(result == 0){ /* timeout */
		return 0;
	}

	if(result > 1){ /* implementation error, only 1 fd was passed*/
		return -1;
	}

	result =  connection_read(socket, read_buffer, count);
	return result;
}

/* write data to the socket, return the number of bytes written*/
int pico_mqtt_connection_write( struct pico_mqtt_socket* socket, void* write_buffer, const uint32_t count, int timeout){
	timeout++; /* to avoid unused error */
	return write(socket->descriptor, write_buffer, count);
}

/* close pico mqtt socket, return -1 on error */
int pico_mqtt_connection_close( struct pico_mqtt_socket* socket){
	int error = 0;
	if( socket == NULL )
		return -1;
	
	error =  close(socket->descriptor);
	free(socket);
	return error;
}

/**
* private function implementation
**/

static struct pico_mqtt_socket* connection_open( const char* URI ){
	const struct addrinfo* addres = resolve_URI( URI );
	struct pico_mqtt_socket* socket = (struct pico_mqtt_socket*) malloc(sizeof(struct pico_mqtt_socket));

	if(addres == NULL){
		return 0;
	}

	socket->descriptor = socket_connect (addres);

	freeaddrinfo((struct addrinfo*) addres); /* check if this is the correct way todo this */

	if(socket->descriptor == -1){
		free(socket);
		return NULL;
	}

	return socket;
}

/* return the socket file descriptor */
static int socket_connect( const struct addrinfo* addres ){
	const struct addrinfo *current_addres;
	int socket_descriptor;
	int result = 0;

	for (current_addres = addres; current_addres != NULL; current_addres = current_addres->ai_next) {
		socket_descriptor = socket(current_addres->ai_family, current_addres->ai_socktype, current_addres->ai_protocol);
	
		if (socket_descriptor == -1)
			continue;
		
		result = connect(socket_descriptor, current_addres->ai_addr, current_addres->ai_addrlen);
		if ( result != -1)
			return socket_descriptor; /* succes */
		
		close(socket_descriptor);
	}
	
	return -1; /* failed */
}

/* return a list of IPv6 addresses */
static const struct addrinfo* resolve_URI( const char* URI ){
	struct addrinfo * res;
	const struct addrinfo hints =  lookup_configuration();
	char* addres_string = (char*) malloc(100);


	/* //TODO specify the service, no hardcoded ports.*/
	int result = getaddrinfo( URI, "1883", &hints, &res );
	printf("resolving the uri \n");
	if(result != 0){
		printf("error %d: %s\n", errno, strerror(errno)); 
		return NULL;
	}

	inet_ntop (res->ai_family, &((struct sockaddr_in6*)res->ai_addr)->sin6_addr ,addres_string, 100);
	printf("resolved URI (%s) to addres: %s\n",URI, addres_string);

	free((void*) addres_string);

	return res;
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
