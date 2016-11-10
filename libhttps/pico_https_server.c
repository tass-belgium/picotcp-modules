/*********************************************************************
   PicoTCP. Copyright (c) 2012 TASS Belgium NV. Some rights reserved.
   See LICENSE and COPYING for usage.

   Author: Andrei Carp <andrei.carp@tass.be>, Alexander Zuliani <alexander.zuliani@tass.be>
 *********************************************************************/

#include "pico_https_server.h"
#include "pico_https_glue.h"
#include "pico_stack.h"
#include "pico_tcp.h"
#include "pico_tree.h"

#define BACKLOG                             10

#define HTTPS_SERVER_CLOSED      0
#define HTTPS_SERVER_LISTEN      1

#define HTTPS_HEADER_MAX_LINE    256u

#define consumeChar(c) (SSL_READ(client->ssl_obj, &(c), 1u))

static const char returnOkHeader[] =
    "HTTP/1.1 200 OK\r\n\
Host: localhost\r\n\
Transfer-Encoding: chunked\r\n\
Connection: close\r\n\
\r\n";

static const char returnOkCacheableHeader[] =
    "HTTP/1.1 200 OK\r\n\
Host: localhost\r\n\
Cache-control: public, max-age=86400\r\n\
Transfer-Encoding: chunked\r\n\
Connection: close\r\n\
\r\n";

static const char returnFailHeader[] =
    "HTTP/1.1 404 Not Found\r\n\
Host: localhost\r\n\
Connection: close\r\n\
\r\n\
<html><body>The resource you requested cannot be found !</body></html>";

static const char errorHeader[] =
    "HTTP/1.1 400 Bad Request\r\n\
Host: localhost\r\n\
Connection: close\r\n\
\r\n\
<html><body>There was a problem with your request !</body></html>";

struct httpsServer
{
    uint16_t state;
    struct pico_socket *sck;
    uint16_t port;
    void (*wakeup)(uint16_t ev, uint16_t param);
    uint8_t accepted;
};

struct httpsClient
{
    uint16_t connectionID;
    struct pico_socket *sck;
    SSL_CONTEXT* ssl_obj;
    void *buffer;
    uint16_t bufferSize;
    uint16_t bufferSent;
    char *resource;
    uint16_t state;
    uint16_t method;
    char *body;
};

/* Local states for clients */
#define HTTPS_HANDSHAKE				 0
#define HTTPS_WAIT_HDR               1
#define HTTPS_WAIT_EOF_HDR           2
#define HTTPS_EOF_HDR                3
#define HTTPS_WAIT_RESPONSE          4
#define HTTPS_WAIT_DATA              5
#define HTTPS_WAIT_STATIC_DATA       6
#define HTTPS_SENDING_DATA           7
#define HTTPS_SENDING_STATIC_DATA    8
#define HTTPS_SENDING_FINAL          9
#define HTTPS_ERROR                  10
#define HTTPS_CLOSED                 11

static struct httpsServer server = {
    0
};

/*
 * Private functions
 */
static int parseRequest(struct httpsClient *client);
static int readRemainingHeader(struct httpsClient *client);
static void sendData(struct httpsClient *client);
static void sendFinal(struct httpsClient *client);
static inline int readData(struct httpsClient *client);  /* used only in one place */
static inline struct httpsClient *findClient(uint16_t conn);


static int compareClients(void *ka, void *kb)
{
    return ((struct httpsClient *)ka)->connectionID - ((struct httpsClient *)kb)->connectionID;
}

static const unsigned char* certificate_buffer = NULL;
static unsigned int certificate_buffer_size = 0;
static const unsigned char* privkey_buffer = NULL;
static unsigned int privkey_buffer_size = 0;

void pico_https_setCertificate(const unsigned char* buffer, int size){
    // Just copy over pointers. Actual sanity checks/initialisations are
    // SSL-implementation specific and should happen at init-time
    certificate_buffer = buffer;
    certificate_buffer_size = size;
}

void pico_https_setPrivateKey(const unsigned char* buffer, int size){
    // Just copy over pointers. Actual sanity checks/initialisations are
    // SSL-implementation specific and should happen at init-time
    privkey_buffer = buffer;
    privkey_buffer_size = size;
}

PICO_TREE_DECLARE(pico_https_clients, compareClients);

void httpsServerCbk(uint16_t ev, struct pico_socket *s)
{
    struct pico_tree_node *index;
    struct httpsClient *client = NULL;
    uint8_t serverEvent = 0u;

    /* determine the client for the socket */
    if( s == server.sck)
    {
        serverEvent = 1u;
    }
    else
    {
		pico_tree_foreach(index, &pico_https_clients)
        {
            client = index->keyValue;
            if(client->sck == s) break;

            client = NULL;
        }
    }

    if(!client && !serverEvent)
    {
        return;
    }

	// Never execute rest of logic until handshake is complete
	if (client && client->state == HTTPS_HANDSHAKE){
		int accState = SSL_HANDSHAKE(client->ssl_obj);	
		if (accState == 0) // Handshake is complete. This part probably needs error handling as well
			client->state = HTTPS_WAIT_HDR;
		return;
	}
	
    if (ev & PICO_SOCK_EV_RD)
    {

        if(readData(client) == HTTPS_RETURN_ERROR)
        {
            /* send out error */
            client->state = HTTPS_ERROR;
            SSL_WRITE(client->ssl_obj, (const char *)errorHeader, sizeof(errorHeader) - 2);
            server.wakeup(EV_HTTPS_ERROR, client->connectionID);
        }
    }

    if(ev & PICO_SOCK_EV_WR)
    {
        if(client->state == HTTPS_SENDING_DATA || client->state == HTTPS_SENDING_STATIC_DATA)
        {
            sendData(client);
        }
        else if(client->state == HTTPS_SENDING_FINAL)
        {
            sendFinal(client);
        }
    }

    if(ev & PICO_SOCK_EV_CONN)
    {
        server.accepted = 0u;
        server.wakeup(EV_HTTPS_CON, HTTPS_SERVER_ID);
        if(!server.accepted)
        {
            pico_socket_close(s); /* reject socket */
        }
    }

    if (ev & PICO_SOCK_EV_FIN) 
    {
		if (client && client->ssl_obj){ 
			SSL_FREE(client->ssl_obj);
			client->ssl_obj = NULL;
		}
    }

    if(ev & PICO_SOCK_EV_CLOSE)
    {
        if (client)
            server.wakeup(EV_HTTPS_CLOSE, (uint16_t)(serverEvent ? HTTPS_SERVER_ID : (client->connectionID)));
    }

    if(ev & PICO_SOCK_EV_ERR)
    {
        server.wakeup(EV_HTTPS_ERROR, (uint16_t)(serverEvent ? HTTPS_SERVER_ID : (client->connectionID)));
		if (client && client->ssl_obj){ 
			SSL_FREE(client->ssl_obj);
			client->ssl_obj = NULL;
		}
    }
}


/*
 * API for starting the server. If 0 is passed as a port, the port 80
 * will be used.
 */


int8_t pico_https_server_start(uint16_t port, void (*wakeup)(uint16_t ev, uint16_t conn))
{

	// Set up pico socket
    struct pico_ip4 anything = {
        0
    };

    server.port = (uint16_t)(port ? short_be(port) : short_be(443u));

    if(!wakeup)
    {
        pico_err = PICO_ERR_EINVAL;
        return HTTPS_RETURN_ERROR;
    }
    
    server.sck = pico_socket_open(PICO_PROTO_IPV4, PICO_PROTO_TCP, &httpsServerCbk);

    if(!server.sck)
    {
        pico_err = PICO_ERR_EFAULT;
        return HTTPS_RETURN_ERROR;
    }

    if(pico_socket_bind(server.sck, &anything, &server.port) != 0)
    {
        pico_err = PICO_ERR_EADDRNOTAVAIL;
        return HTTPS_RETURN_ERROR;
    }

    if (pico_socket_listen(server.sck, BACKLOG) != 0)
    {
        pico_err = PICO_ERR_EADDRINUSE;
        return HTTPS_RETURN_ERROR;
    }

    // Glue should implement this
    if(pico_https_ssl_init(certificate_buffer, certificate_buffer_size, privkey_buffer, privkey_buffer_size) != 0) {
        pico_err = PICO_ERR_EFAULT;
        return HTTPS_RETURN_ERROR;
    }

	// Some state
    server.wakeup = wakeup;
    server.state = HTTPS_SERVER_LISTEN;
	
    return HTTPS_RETURN_OK;
}

/*
 * API for accepting new connections. This function should be
 * called when the event EV_HTTP_CON is triggered, if not called
 * when noticed the connection will be considered rejected and the
 * socket will be dropped.
 *
 * Returns the ID of the new connection or a negative value if error.
 */
int pico_https_server_accept(void)
{
    struct pico_ip4 orig;
    struct httpsClient *client;
    uint16_t port;

    client = PICO_ZALLOC(sizeof(struct httpsClient));
    if(!client)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTPS_RETURN_ERROR;
    }

    client->sck = pico_socket_accept(server.sck, &orig, &port);

    if(!client->sck)
    {
        pico_err = PICO_ERR_ENOMEM;
        PICO_FREE(client);
        return HTTPS_RETURN_ERROR;
    }

    // Error handling?
    client->ssl_obj = pico_https_ssl_accept(client->sck);

	if (client->ssl_obj){
		server.accepted=1u;
    	client->state = HTTPS_HANDSHAKE;
    	client->buffer = NULL;
    	client->bufferSize = 0;
    	client->body = NULL;
    	client->connectionID = pico_rand() & 0x7FFF;
	}else{ // Failed to initialise. Kill this socket now!
		pico_socket_close(client->sck);
	}

    /* add element to the tree, if duplicate because the rand */
    /* regenerate */
    while(pico_tree_insert(&pico_https_clients, client) != NULL)
        client->connectionID = pico_rand() & 0x7FFF;
    return client->connectionID;
}

/*
 * Function used for getting the resource asked by the
 * client. It is useful after the request header (EV_HTTP_REQ)
 * from client was received, otherwise NULL is returned.
 */
char *pico_https_getResource(uint16_t conn)
{
    struct httpsClient *client = findClient(conn);

    if(!client)
        return NULL;
    else
        return client->resource;
}

/*
 * Function used for getting the method coming from client
 * (e.g. POST, GET...)
 * Should only be used after header was read (EV_HTTP_REQ)
 */

int pico_https_getMethod(uint16_t conn)
{
    struct httpsClient *client = findClient(conn);

    if(!client)
        return 0;
    else
        return client->method;
}

/*
 * Function used for getting the body of the request header
 * It is useful after a POST request header (EV_HTTP_REQ)
 * from client was received, otherwise NULL is returned.
 */
char *pico_https_getBody(uint16_t conn)
{
    struct httpsClient *client = findClient(conn);

    if(!client)
        return NULL;
    else
        return client->body;
}


/*
 * After the resource was asked by the client (EV_HTTP_REQ)
 * before doing anything else, the server has to let know
 * the client if the resource can be provided or not.
 *
 * This is controlled via the code parameter which can
 * have three values :
 *
 * HTTP_RESOURCE_FOUND, HTTP_STATIC_RESOURCE_FOUND or HTTP_RESOURCE_NOT_FOUND
 *
 * If a resource is reported not found the 404 header will be sent and the connection
 * will be closed , otherwise the 200 header is sent and the user should
 * immediately submit (static) data.
 *
 */
int pico_https_respond(uint16_t conn, uint16_t code)
{
    struct httpsClient *client = findClient(conn);

    if(!client)
    {
        dbg("Client not found !\n");
        return HTTPS_RETURN_ERROR;
    }

    if(client->state == HTTPS_WAIT_RESPONSE)
    {
        if(code & HTTPS_RESOURCE_FOUND)
        {
            client->state = (code & HTTPS_STATIC_RESOURCE) ? HTTPS_WAIT_STATIC_DATA : HTTPS_WAIT_DATA;
            if(code & HTTPS_CACHEABLE_RESOURCE)
            {
                return SSL_WRITE(client->ssl_obj, (const char *)returnOkCacheableHeader, sizeof(returnOkCacheableHeader) - 1); /* remove \0 */
            }
            else
            {
                return SSL_WRITE(client->ssl_obj, (const char *)returnOkHeader, sizeof(returnOkHeader) - 1); /* remove \0 */
            }
        }
        else
        {
            int length;

            length = SSL_WRITE(client->ssl_obj, (const char *)returnFailHeader, sizeof(returnFailHeader) - 1); /* remove \0 */
			SSL_SHUTDOWN(client->ssl_obj);
            pico_socket_close(client->sck);
            client->state = HTTPS_CLOSED;
            return length;

        }
    }
    else
    {
        dbg("Bad state for the client \n");
        return HTTPS_RETURN_ERROR;
    }

}

/*
 * API used to submit data to the client.
 * Server sends data only using Transfer-Encoding: chunked.
 *
 * With this function the user will submit a data chunk to
 * be sent. If it's static data the function will not allocate a buffer.
 * The function will send the chunk size in hex and the rest will
 * be sent using WR event from sockets.
 * After each transmision EV_HTTP_PROGRESS is called and at the
 * end of the chunk EV_HTTP_SENT is called.
 *
 * To let the client know this is the last chunk, the user
 * should pass a NULL buffer.
 */
int8_t pico_https_submitData(uint16_t conn, void *buffer, uint16_t len)
{

    struct httpsClient *client = findClient(conn);
    char chunkStr[10];
    int chunkCount;

    if(!client)
    {
        dbg("Wrong connection ID\n");
        return HTTPS_RETURN_ERROR;
    }

    if(client->state != HTTPS_WAIT_DATA && client->state != HTTPS_WAIT_STATIC_DATA)
    {
        dbg("Client is in a different state than accepted\n");
        return HTTPS_RETURN_ERROR;
    }

    if(client->buffer)
    {
        dbg("Already a buffer submited\n");
        return HTTPS_RETURN_ERROR;
    }


    if(!buffer)
    {
        len = 0;
    }

    if(len > 0)
    {
        if(client->state == HTTPS_WAIT_STATIC_DATA)
        {
            client->buffer = buffer;
        }
        else
        {
            client->buffer = PICO_ZALLOC(len);
            if(!client->buffer)
            {
                pico_err = PICO_ERR_ENOMEM;
                return HTTPS_RETURN_ERROR;
            }

            /* taking over the buffer */
            memcpy(client->buffer, buffer, len);
        }
    }
    else
        client->buffer = NULL;


    client->bufferSize = len;
    client->bufferSent = 0;

    /* create the chunk size and send it */
    if(len > 0)
    {
        client->state = (client->state == HTTPS_WAIT_DATA) ? HTTPS_SENDING_DATA : HTTPS_SENDING_STATIC_DATA;
        chunkCount = pico_itoaHex(client->bufferSize, chunkStr);
        chunkStr[chunkCount++] = '\r';
        chunkStr[chunkCount++] = '\n';
        SSL_WRITE(client->ssl_obj, chunkStr, chunkCount); // We assume this succeeds. Bad idea.
    }
    else
    {
        sendFinal(client);
    }

    return HTTPS_RETURN_OK;
}

/*
 * When EV_HTTP_PROGRESS is triggered you can use this
 * function to check the state of the chunk.
 */

int pico_https_getProgress(uint16_t conn, uint16_t *sent, uint16_t *total)
{
    struct httpsClient *client = findClient(conn);

    if(!client)
    {
        dbg("Wrong connection id !\n");
        return HTTPS_RETURN_ERROR;
    }

    *sent = client->bufferSent;
    *total = client->bufferSize;

    return HTTPS_RETURN_OK;
}

/*
 * This API can be used to close either a client
 * or the server ( if you pass HTTP_SERVER_ID as a connection ID).
 */
int pico_https_close(uint16_t conn)
{
    /* close the server */
    if(conn == HTTPS_SERVER_ID)
    {
        if(server.state == HTTPS_SERVER_LISTEN)
        {
            struct pico_tree_node *index, *tmp;
            /* close the server */
            pico_socket_close(server.sck);
            server.sck = NULL;

            /* destroy the tree */
            pico_tree_foreach_safe(index, &pico_https_clients, tmp)
            {
                struct httpsClient *client = index->keyValue;

                if(client->resource)
                    PICO_FREE(client->resource);

                if(client->body)
                    PICO_FREE(client->body);

                pico_socket_close(client->sck);
                pico_tree_delete(&pico_https_clients, client);
            }

            server.state = HTTPS_SERVER_CLOSED;
            return HTTPS_RETURN_OK;
        }
        else /* nothing to close */
            return HTTPS_RETURN_ERROR;
    } /* close a connection in this case */
    else
    {
        struct httpsClient *client = findClient(conn);

        if(!client)
        {
            dbg("Client not found..\n");
            return HTTPS_RETURN_ERROR;
        }

        pico_tree_delete(&pico_https_clients, client);

        if(client->resource)
            PICO_FREE(client->resource);

        if(client->state != HTTPS_SENDING_STATIC_DATA && client->buffer)
            PICO_FREE(client->buffer);

        if(client->body)
            PICO_FREE(client->body);

		if(client->ssl_obj){
			SSL_FREE(client->ssl_obj);
			client->ssl_obj=NULL;
		}

        if(client->state != HTTPS_CLOSED || !client->sck) {
            pico_socket_close(client->sck);
            client->sck = NULL;
        }

        PICO_FREE(client);
        return HTTPS_RETURN_OK;
    }
}

static int parseRequestConsumeFullLine(struct httpsClient *client, char *line)
{
    char c = 0;
    uint32_t index = 0;
    /* consume the full line */
    while(consumeChar(c) > 0) /* read char by char only the first line */
    {
        line[++index] = c;
        if(c == '\n')
            break;

        if(index >= HTTPS_HEADER_MAX_LINE)
        {
            dbg("Size exceeded \n");
            return HTTPS_RETURN_ERROR;
        }
    }
    return (int)index;
}

static int parseRequestExtractFunction(char *line, int index, const char *method)
{
    uint8_t len = (uint8_t)strlen(method);

    /* extract the function and the resource */
    if(memcmp(line, method, len) || line[len] != ' ' || index < 10 || line[index] != '\n')
    {
        dbg("Wrong command or wrong ending\n");
        return HTTPS_RETURN_ERROR;
    }

    return 0;
}

static int parseRequestReadResource(struct httpsClient *client, int method_length, char *line)
{
    uint32_t index;

    /* start reading the resource */
    index = (uint32_t)method_length + 1; /* go after ' ' */
    while(line[index] != ' ')
    {
        if(line[index] == '\n') /* no terminator ' ' */
        {
            dbg("No terminator...\n");
            return HTTPS_RETURN_ERROR;
        }

        index++;
    }
    client->resource = PICO_ZALLOC(index - (uint32_t)method_length); /* allocate without the method in front + 1 which is \0 */

    if(!client->resource)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTPS_RETURN_ERROR;
    }

    /* copy the resource */
    memcpy(client->resource, line + method_length + 1, index - (uint32_t)method_length - 1); /* copy without the \0 which was already set by PICO_ZALLOC */
    return 0;
}

static int parseRequestGet(struct httpsClient *client, char *line)
{
    int ret;

    ret = parseRequestConsumeFullLine(client, line);
    if(ret < 0)
        return ret;

    ret = parseRequestExtractFunction(line, ret, "GET");
    if(ret)
        return ret;

    ret = parseRequestReadResource(client, strlen("GET"), line);
    if(ret)
        return ret;

    client->state = HTTPS_WAIT_EOF_HDR;
    client->method = HTTPS_METHOD_GET;
    return HTTPS_RETURN_OK;
}

static int parseRequestPost(struct httpsClient *client, char *line)
{
    int ret;

    ret = parseRequestConsumeFullLine(client, line);
    if(ret < 0)
        return ret;

    ret = parseRequestExtractFunction(line, ret, "POST");
    if(ret)
        return ret;

    ret = parseRequestReadResource(client, strlen("POST"), line);
    if(ret)
        return ret;

    client->state = HTTPS_WAIT_EOF_HDR;
    client->method = HTTPS_METHOD_POST;
    return HTTPS_RETURN_OK;
}

/* check the integrity of the request */
int parseRequest(struct httpsClient *client)
{
    char c = 0;
    char line[HTTPS_HEADER_MAX_LINE];
    /* read first line */
    if (consumeChar(c) < 0) return 0; 
    line[0] = c;
    if(c == 'G')
    { /* possible GET */
        return parseRequestGet(client, line);
    }
    else if(c == 'P')
    { /* possible POST */
        return parseRequestPost(client, line);
    }

    return HTTPS_RETURN_ERROR;
}

int readRemainingHeader(struct httpsClient *client)
{
    char line[1000];
    int count = 0;
    int len;

    while((len = SSL_READ(client->ssl_obj, line, 1000u)) > 0)
    {
        char c;
        int index = 0;
        uint32_t body_len = 0;
        /* parse the response */
        while(index < len)
        {
            c = line[index++];
            if(c != '\r' && c != '\n')
                count++;

            if(c == '\n')
            {
                if(!count)
                {
                    client->state = HTTPS_EOF_HDR;
                    /*dbg("End of header !\n");*/

                    body_len = (uint32_t)(len - index);
                    if(body_len > 0)
                    {
                        client->body = PICO_ZALLOC(body_len + 1u);
                        if(client->body)
                        {
                            memcpy(client->body, line + index, body_len);
                        }
                        else
                        {
                            return HTTPS_RETURN_ERROR;
                        }
                    }

                    break;
                }

                count = 0;

            }
        }
    }
    return HTTPS_RETURN_OK;
}

void sendData(struct httpsClient *client)
{
    int length;
    while( client->bufferSent < client->bufferSize &&
           (length = SSL_WRITE(client->ssl_obj, (uint8_t *)client->buffer + client->bufferSent, \
                                                 client->bufferSize - client->bufferSent)) > 0 )
    {
        client->bufferSent = (uint16_t)(client->bufferSent + length);
        server.wakeup(EV_HTTPS_PROGRESS, client->connectionID);
    }
    if(client->bufferSent == client->bufferSize && client->bufferSize)
    {
        /* send chunk trail */
        if(SSL_WRITE(client->ssl_obj, "\r\n", 2) > 0)
        {
            /* free the buffer */
            if(client->state == HTTPS_SENDING_DATA)
            {
                PICO_FREE(client->buffer);
            }

            client->buffer = NULL;

            client->state = HTTPS_WAIT_DATA;
            server.wakeup(EV_HTTPS_SENT, client->connectionID);
        }
    }

}

void sendFinal(struct httpsClient *client)
{
    if(SSL_WRITE(client->ssl_obj, "0\r\n\r\n", 5u) > 0)
    {
		SSL_SHUTDOWN(client->ssl_obj);
        pico_socket_close(client->sck);
        client->state = HTTPS_CLOSED;
    }
    else
    {
        client->state = HTTPS_SENDING_FINAL;
    }
}

int readData(struct httpsClient *client)
{
    if(!client)
    {
        dbg("Wrong connection ID\n");
        return HTTPS_RETURN_ERROR;
    }
	
    if(client->state == HTTPS_WAIT_HDR)
    {
        if(parseRequest(client) < 0 || readRemainingHeader(client) < 0)
        {
            return HTTPS_RETURN_ERROR;
        }
    } /* continue with this in case the header comes line by line not a big chunk */
    else if(client->state == HTTPS_WAIT_EOF_HDR)
    {
        if(readRemainingHeader(client) < 0 )
            return HTTPS_RETURN_ERROR;
    }

    if(client->state == HTTPS_EOF_HDR)
    {
        client->state = HTTPS_WAIT_RESPONSE;
        server.wakeup(EV_HTTPS_REQ, client->connectionID);
    }

    return HTTPS_RETURN_OK;
}

struct httpsClient *findClient(uint16_t conn)
{
    struct httpsClient dummy = {
        .connectionID = conn
    };

    return pico_tree_findKey(&pico_https_clients, &dummy);
}
