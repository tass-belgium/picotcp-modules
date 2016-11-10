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
#include "../pico_mqtt_debug.h"
#include "../pico_mqtt_socket.h"

/**
* data structures
**/

struct pico_mqtt_socket{
    int descriptor;
};

/**
* Private function prototypes
**/

int pico_mqtt_connection_bind(struct pico_mqtt_socket** connection, const char* uri, const char* port);
static int allocate_socket( struct pico_mqtt_socket** socket);
static struct addrinfo lookup_configuration_server( void );
static int resolve_uri_server( struct addrinfo** addresses, const char* uri, const char* port);
static int socket_bind( struct pico_mqtt_socket* connection, struct addrinfo* addres );

void error(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int result = 0;

    int n;
    int newsockfd = 0;
    int flag = 1;
    uint8_t received_byte = 0;
    struct sockaddr_in6 client_address;
    socklen_t client_address_length;
    char client_addr_ipv6[100];
    struct pollfd socket_poll;


    struct pico_mqtt_socket* connection = NULL;

    result = pico_mqtt_connection_bind( &connection, "127.0.0.1", "3200");
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
    
    socket_poll = (struct pollfd) {
        .fd = connection->descriptor,
        .events = POLLIN,
        .revents = 0
    };

    result = poll(&socket_poll, 1, -1);
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
        error("ERROR on accept");

    //Sockets Layer Call: inet_ntop()
    inet_ntop(AF_INET6, &(client_address.sin6_addr),client_addr_ipv6, 100);
    printf("Incoming connection from client having IPv6 address: %s\n",client_addr_ipv6);
    
    //Sockets Layer Call: recv()
    socket_poll = (struct pollfd) {
        .fd = newsockfd,
        .events = POLLIN,
        .revents = 0
    };
    while(flag)
    {
        //result = poll(&socket_poll, 1, -1);

        n = read(newsockfd, &received_byte, 1);
        
        if (n < 0)
        {
            PERROR("Reading from socket returned %d: %s\n", errno, strerror(errno));
            return ERROR;
        }

        if (n == 0)
        {
            printf("\n");
            PINFO("End of file, done reading.\n");
            flag = 0;
            continue;
        }

        printf("%c", (char)received_byte);
    }

    //Sockets Layer Call: send()
    n = send(newsockfd, "Server got your message", 23+1, 0);
    if (n < 0)
        error("ERROR writing to socket");
    
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

    result = allocate_socket(connection);
    if(result == ERROR)
    {
        return ERROR;
    }

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
