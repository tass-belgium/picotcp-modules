/*********************************************************************
   PicoTCP. Copyright (c) 2012 TASS Belgium NV. Some rights reserved.
   See LICENSE and COPYING for usage.

   Author: Andrei Carp <andrei.carp@tass.be>
 *********************************************************************/
#include "pico_stack.h"
#include "pico_http_server.h"
#include "pico_tcp.h"
#include "pico_tree.h"
#include "pico_socket.h"

#define BACKLOG                             10

#define HTTP_SERVER_CLOSED      0
#define HTTP_SERVER_LISTEN      1

#define HTTP_HEADER_MAX_LINE    256u
#define HTTP_OK_HEADER_FIXED    160u

#define consume_char(c) (pico_socket_read(client->sck, &c, 1u))

//TODO: check in rfc what to add

static const char return_fail_header[] =
    "HTTP/1.1 404 Not Found\r\n\
Host: localhost\r\n\
Connection: close\r\n\
\r\n\
<html><body>The resource you requested cannot be found !</body></html>";

static const char error_header[] =
    "HTTP/1.1 400 Bad Request\r\n\
Host: localhost\r\n\
Connection: close\r\n\
\r\n\
<html><body>There was a problem with your request !</body></html>";


int32_t construct_return_ok_header(char* headerstring, uint8_t cacheable, const char* contenttype)
{
    strcat(headerstring, "HTTP/1.1 200 OK\r\n");
    strcat(headerstring, "Host: localhost\r\n");
    if (cacheable == HTTP_CACHEABLE_RESOURCE)
    {
        strcat(headerstring, "Cache-control: public, max-age=86400\r\n");
    }
    if (contenttype != NULL)
    {
        sprintf(headerstring, "%sContent-Type: %s\r\n", headerstring, contenttype);
    }
    strcat(headerstring, "Transfer-Encoding: chunked\r\n");
    strcat(headerstring, "Connection: close\r\n");
    strcat(headerstring, "\r\n");
    return strlen(headerstring);
}


struct http_server
{
    uint16_t state;
    struct pico_socket *sck;
    uint16_t port;
    void (*wakeup)(uint16_t ev, uint16_t param);
    uint8_t accepted;
};

struct http_client
{
    uint16_t connectionID;
    struct pico_socket *sck;
    void *buffer;
    uint16_t buffer_size;
    uint16_t buffer_sent;
    char *resource;
    uint16_t state;
    uint16_t method;
    char *body;
};

/* Local states for clients */
#define HTTP_WAIT_HDR               0
#define HTTP_WAIT_EOF_HDR           1
#define HTTP_EOF_HDR                2
#define HTTP_WAIT_RESPONSE          3
#define HTTP_WAIT_DATA              4
#define HTTP_WAIT_STATIC_DATA       5
#define HTTP_SENDING_DATA           6
#define HTTP_SENDING_STATIC_DATA    7
#define HTTP_SENDING_FINAL          8
#define HTTP_ERROR                  9
#define HTTP_CLOSED                 10

static struct http_server server = {
    0
};

/*
 * Private functions
 */
static int16_t parse_request(struct http_client *client);
//static int read_remaining_reader(struct http_client *client);
static void send_data(struct http_client *client);
static void send_final(struct http_client *client);
static inline int32_t read_data(struct http_client *client);  /* used only in a place */
static inline struct http_client *find_client(uint16_t conn);



static int32_t compare_clients(void *ka, void *kb)
{
    return ((struct http_client *)ka)->connectionID - ((struct http_client *)kb)->connectionID;
}

PICO_TREE_DECLARE(pico_http_clients, compare_clients);

void http_server_cbk(uint16_t ev, struct pico_socket *s)
{
    struct pico_tree_node *index;
    struct http_client *client = NULL;
    uint8_t server_event = 0u;

    /* determine the client for the socket */
    if (s == server.sck)
    {
        server_event = 1u;
    }
    else
    {
        pico_tree_foreach(index, &pico_http_clients)
        {
            client = index->keyValue;
            if (client->sck == s) break;

            client = NULL;
        }
    }

    if (!client && !server_event)
    {
        return;
    }

    if (ev & PICO_SOCK_EV_RD)
    {

        if (read_data(client) == HTTP_RETURN_ERROR)
        {
            /* send out error */
            client->state = HTTP_ERROR;
            pico_socket_write(client->sck, (const char *)error_header, sizeof(error_header) - 1);
            server.wakeup(EV_HTTP_ERROR, client->connectionID);
        }
    }

    if (ev & PICO_SOCK_EV_WR)
    {
        if (client->state == HTTP_SENDING_DATA || client->state == HTTP_SENDING_STATIC_DATA)
        {
            send_data(client);
        }
        else if (client->state == HTTP_SENDING_FINAL)
        {
            send_final(client);
        }
    }

    if (ev & PICO_SOCK_EV_CONN)
    {
        server.accepted = 0u;
        server.wakeup(EV_HTTP_CON, HTTP_SERVER_ID);
        if (!server.accepted)
        {
            pico_socket_close(s); /* reject socket */
        }
    }

    if ((ev & PICO_SOCK_EV_CLOSE) || (ev & PICO_SOCK_EV_FIN))
    {
        server.wakeup(EV_HTTP_CLOSE, (uint16_t)(server_event ? HTTP_SERVER_ID : (client->connectionID)));
    }

    if (ev & PICO_SOCK_EV_ERR)
    {
        server.wakeup(EV_HTTP_ERROR, (uint16_t)(server_event ? HTTP_SERVER_ID : (client->connectionID)));
    }
}

/*
 * API for starting the server. If 0 is passed as a port, the port 80
 * will be used.
 */
int16_t pico_http_server_start(uint16_t port, void (*wakeup)(uint16_t ev, uint16_t conn))
{
    struct pico_ip4 anything = {
        0
    };

    server.port = (uint16_t)(port ? short_be(port) : short_be(80u));

    if (!wakeup)
    {
        pico_err = PICO_ERR_EINVAL;
        return HTTP_RETURN_ERROR;
    }

    server.sck = pico_socket_open(PICO_PROTO_IPV4, PICO_PROTO_TCP, &http_server_cbk);

    if (!server.sck)
    {
        pico_err = PICO_ERR_EFAULT;
        return HTTP_RETURN_ERROR;
    }

    if (pico_socket_bind(server.sck, &anything, &server.port) != 0)
    {
        pico_err = PICO_ERR_EADDRNOTAVAIL;
        return HTTP_RETURN_ERROR;
    }

    if (pico_socket_listen(server.sck, BACKLOG) != 0)
    {
        pico_err = PICO_ERR_EADDRINUSE;
        return HTTP_RETURN_ERROR;
    }

    server.wakeup = wakeup;
    server.state = HTTP_SERVER_LISTEN;
    return HTTP_RETURN_OK;
}

/*
 * API for accepting new connections. This function should be
 * called when the event EV_HTTP_CON is triggered, if not called
 * when noticed the connection will be considered rejected and the
 * socket will be dropped.
 *
 * Returns the ID of the new connection or a negative value if error.
 */
int32_t pico_http_server_accept(void)
{
    struct pico_ip4 orig;
    struct http_client *client;
    uint16_t port;

    client = PICO_ZALLOC(sizeof(struct http_client));
    if (!client)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }

    client->sck = pico_socket_accept(server.sck, &orig, &port);

    if (!client->sck)
    {
        pico_err = PICO_ERR_ENOMEM;
        PICO_FREE(client);
        return HTTP_RETURN_ERROR;
    }

    server.accepted = 1u;
    /* buffer used for async sending */
    client->state = HTTP_WAIT_HDR;
    client->buffer = NULL;
    client->buffer_size = 0;
    client->body = NULL;
    client->connectionID = pico_rand() & 0x7FFF;

    /* add element to the tree, if duplicate because the rand */
    /* regenerate */
    while (pico_tree_insert(&pico_http_clients, client) != NULL)
        client->connectionID = pico_rand() & 0x7FFF;
    return client->connectionID;
}

/*
 * Function used for getting the resource asked by the
 * client. It is useful after the request header (EV_HTTP_REQ)
 * from client was received, otherwise NULL is returned.
 */
char *pico_http_get_resource(uint16_t conn)
{
    struct http_client *client = find_client(conn);

    if (!client)
        return NULL;
    else
        return client->resource;
}

/*
 * Function used for getting the method coming from client
 * (e.g. POST, GET...)
 * Should only be used after header was read (EV_HTTP_REQ)
 */

int16_t pico_http_get_method(uint16_t conn)
{
    struct http_client *client = find_client(conn);

    if (!client)
        return 0;
    else
        return client->method;
}

/*
 * Function used for getting the body of the request header
 * It is useful after a POST request header (EV_HTTP_REQ)
 * from client was received, otherwise NULL is returned.
 */
char *pico_http_get_body(uint16_t conn)
{
    struct http_client *client = find_client(conn);

    if (!client)
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
int32_t pico_http_respond_mimetype(uint16_t conn, uint16_t code, const char* mimetype)
{
    struct http_client *client = find_client(conn);

    if (!client)
    {
        dbg("Client not found !\n");
        return HTTP_RETURN_ERROR;
    }

    if (client->state == HTTP_WAIT_RESPONSE)
    {
        if (code & HTTP_RESOURCE_FOUND)
        {
            client->state = (code & HTTP_STATIC_RESOURCE) ? HTTP_WAIT_STATIC_DATA : HTTP_WAIT_DATA;
            uint16_t len = HTTP_OK_HEADER_FIXED;
            if (mimetype != NULL)
                len += (uint16_t)strlen(mimetype);
            char *retheader = PICO_ZALLOC(len);
            if (!retheader)
            {
                pico_err = PICO_ERR_ENOMEM;
                return HTTP_RETURN_ERROR;
            }
            if (code & HTTP_CACHEABLE_RESOURCE)
            {
                int32_t length = construct_return_ok_header(retheader, HTTP_CACHEABLE_RESOURCE, mimetype);
                uint8_t rv = pico_socket_write(client->sck, retheader, length ); /* remove \0 */
                PICO_FREE(retheader);
                return rv;
            }
            else
            {
                int32_t length = construct_return_ok_header(retheader, HTTP_STATIC_RESOURCE, mimetype);
                uint8_t rv = pico_socket_write(client->sck, retheader, length ); /* remove \0 */
                PICO_FREE(retheader);
                return rv;
            }
        }
        else
        {
            int32_t length;
            length = pico_socket_write(client->sck, (const uint8_t *)return_fail_header, sizeof(return_fail_header) - 1); /* remove \0 */
            pico_socket_close(client->sck);
            client->state = HTTP_CLOSED;
            return length;
        }
    }
    else
    {
        dbg("Bad state for the client \n");
        return HTTP_RETURN_ERROR;
    }
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
int32_t pico_http_respond(uint16_t conn, uint16_t code)
{
    struct http_client *client = find_client(conn);

    if (!client)
    {
        dbg("Client not found !\n");
        return HTTP_RETURN_ERROR;
    }

    if (client->state == HTTP_WAIT_RESPONSE)
    {
        if (code & HTTP_RESOURCE_FOUND)
        {
            client->state = (code & HTTP_STATIC_RESOURCE) ? HTTP_WAIT_STATIC_DATA : HTTP_WAIT_DATA;

            /* Try to guess MIME type */
            const char* mimetype = pico_http_get_mimetype(client->resource);

            uint16_t len = HTTP_OK_HEADER_FIXED;
            if (mimetype != NULL)
                len += (uint16_t)strlen(mimetype);
            char *retheader = PICO_ZALLOC(len);
            if (!retheader)
            {
                pico_err = PICO_ERR_ENOMEM;
                return HTTP_RETURN_ERROR;
            }

            if (code & HTTP_CACHEABLE_RESOURCE)
            {
                int32_t length = construct_return_ok_header(retheader, HTTP_CACHEABLE_RESOURCE, mimetype);
                uint8_t rv = pico_socket_write(client->sck, retheader, length ); /* remove \0 */
                PICO_FREE(retheader);
                retheader = NULL;
                return rv;
            }
            else
            {
                int32_t length = construct_return_ok_header(retheader, HTTP_STATIC_RESOURCE, mimetype);
                uint8_t rv = pico_socket_write(client->sck, retheader, length ); /* remove \0 */
                PICO_FREE(retheader);
                retheader = NULL;
                return rv;
            }
        }
        else
        {
            int32_t length;

            length = pico_socket_write(client->sck, (const uint8_t *)return_fail_header, sizeof(return_fail_header) - 1); /* remove \0 */
            pico_socket_close(client->sck);
            client->state = HTTP_CLOSED;
            return length;

        }
    }
    else
    {
        dbg("Bad state for the client \n");
        return HTTP_RETURN_ERROR;
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
int16_t pico_http_submit_data(uint16_t conn, void *buffer, uint16_t len)
{

    struct http_client *client = find_client(conn);
    char chunk_str[10];
    int16_t chunk_count;

    if (!client)
    {
        dbg("Wrong connection ID\n");
        return HTTP_RETURN_ERROR;
    }

    if (client->state != HTTP_WAIT_DATA && client->state != HTTP_WAIT_STATIC_DATA)
    {
        dbg("Client is in a different state than accepted\n");
        return HTTP_RETURN_ERROR;
    }

    if (client->buffer)
    {
        dbg("Already a buffer submited\n");
        return HTTP_RETURN_ERROR;
    }

    if (!buffer)
    {
        len = 0;
    }

    if (len > 0)
    {
        if (client->state == HTTP_WAIT_STATIC_DATA)
        {
            client->buffer = buffer;
        }
        else
        {
            client->buffer = PICO_ZALLOC(len);
            if (!client->buffer)
            {
                pico_err = PICO_ERR_ENOMEM;
                return HTTP_RETURN_ERROR;
            }

            /* taking over the buffer */
            memcpy(client->buffer, buffer, len);
        }
    }
    else
        client->buffer = NULL;


    client->buffer_size = len;
    client->buffer_sent = 0;

    /* create the chunk size and send it */
    if (len > 0)
    {
        client->state = (client->state == HTTP_WAIT_DATA) ? HTTP_SENDING_DATA : HTTP_SENDING_STATIC_DATA;
        chunk_count = pico_itoaHex(client->buffer_size, chunk_str);
        chunk_str[chunk_count++] = '\r';
        chunk_str[chunk_count++] = '\n';
        pico_socket_write(client->sck, chunk_str, chunk_count);
    }
    else
    {
        send_final(client);
    }

    return HTTP_RETURN_OK;
}

/*
 * When EV_HTTP_PROGRESS is triggered you can use this
 * function to check the state of the chunk.
 */

int16_t pico_http_get_progress(uint16_t conn, uint16_t *sent, uint16_t *total)
{
    struct http_client *client = find_client(conn);

    if(!client)
    {
        dbg("Wrong connection id !\n");
        return HTTP_RETURN_ERROR;
    }

    *sent = client->buffer_sent;
    *total = client->buffer_size;

    return HTTP_RETURN_OK;
}

/*
 * This API can be used to close either a client
 * or the server ( if you pass HTTP_SERVER_ID as a connection ID).
 */
int16_t pico_http_close(uint16_t conn)
{
    /* close the server */
    if (conn == HTTP_SERVER_ID)
    {
        if (server.state == HTTP_SERVER_LISTEN)
        {
            struct pico_tree_node *index, *tmp;
            /* close the server */
            pico_socket_close(server.sck);
            server.sck = NULL;

            /* destroy the tree */
            pico_tree_foreach_safe(index, &pico_http_clients, tmp)
            {
                struct http_client *client = index->keyValue;

                if (client->resource)
                    PICO_FREE(client->resource);

                if (client->body)
                    PICO_FREE(client->body);

                pico_socket_close(client->sck);
                pico_tree_delete(&pico_http_clients, client);
            }

            server.state = HTTP_SERVER_CLOSED;
            return HTTP_RETURN_OK;
        }
        else /* nothing to close */
            return HTTP_RETURN_ERROR;
    } /* close a connection in this case */
    else
    {
        struct http_client *client = find_client(conn);

        if (!client)
        {
            dbg("Client not found..\n");
            return HTTP_RETURN_ERROR;
        }

        pico_tree_delete(&pico_http_clients, client);

        if (client->resource)
            PICO_FREE(client->resource);

        if (client->state != HTTP_SENDING_STATIC_DATA && client->buffer)
            PICO_FREE(client->buffer);

        if (client->body)
            PICO_FREE(client->body);

        if (client->state != HTTP_CLOSED || !client->sck)
            pico_socket_close(client->sck);

        PICO_FREE(client);
        return HTTP_RETURN_OK;
    }
}

static int32_t parse_request_consume_full_line(struct http_client *client, char *line)
{
    char c = 0;
    uint32_t index = 0;
    /* consume the full line */
    while (consume_char(c) > 0) /* read char by char only the first line */
    {
        line[++index] = c;
        if (c == '\n')
            break;

        if (index >= HTTP_HEADER_MAX_LINE)
        {
            dbg("Size exceeded \n");
            return HTTP_RETURN_ERROR;
        }
    }
    return (int32_t)index;
}

static uint16_t parse_request_extract_function(char *line, uint8_t index, const char *method)
{
    uint8_t len = (uint8_t)strlen(method);

    /* extract the function and the resource */
    if (memcmp(line, method, len) || line[len] != ' ' || index < 10 || line[index] != '\n')
    {
        dbg("Wrong command or wrong ending\n");
        return HTTP_RETURN_ERROR;
    }

    return 0;
}

static int16_t parse_request_read_resource(struct http_client *client, uint32_t method_length, char *line)
{
    uint32_t index;

    /* start reading the resource */
    index = (uint32_t)method_length + 1; /* go after ' ' */
    while (line[index] != ' ')
    {
        if (line[index] == '\n') /* no terminator ' ' */
        {
            dbg("No terminator...\n");
            return HTTP_RETURN_ERROR;
        }

        index++;
    }
    client->resource = PICO_ZALLOC(index - (uint32_t)method_length); /* allocate without the method in front + 1 which is \0 */

    if (!client->resource)
    {
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }

    /* copy the resource */
    memcpy(client->resource, line + method_length + 1, index - (uint32_t)method_length - 1); /* copy without the \0 which was already set by PICO_ZALLOC */
    return 0;
}

static int32_t parse_request_get(struct http_client *client, char *line)
{
    int32_t ret;

    ret = parse_request_consume_full_line(client, line);
    if (ret < 0)
        return ret;

    ret = parse_request_extract_function(line, ret, "GET");
    if (ret)
        return ret;

    ret = parse_request_read_resource(client, strlen("GET"), line);
    if (ret)
        return ret;

    client->state = HTTP_WAIT_EOF_HDR;
    client->method = HTTP_METHOD_GET;
    return HTTP_RETURN_OK;
}

static int32_t parse_request_post(struct http_client *client, char *line)
{
    int32_t ret;

    ret = parse_request_consume_full_line(client, line);
    if (ret < 0)
        return ret;

    ret = parse_request_extract_function(line, ret, "POST");
    if (ret)
        return ret;

    ret = parse_request_read_resource(client, strlen("POST"), line);
    if (ret)
        return ret;

    client->state = HTTP_WAIT_EOF_HDR;
    client->method = HTTP_METHOD_POST;
    return HTTP_RETURN_OK;
}

/* check the integrity of the request */
int16_t parse_request(struct http_client *client)
{
    uint8_t c = 0;
    char *line = (char *)PICO_ZALLOC(HTTP_HEADER_MAX_LINE);
    if (!line)
        {
            pico_err = PICO_ERR_ENOMEM;
            return HTTP_RETURN_ERROR;
        }

    /* read first line */
    consume_char(c);
    line[0] = c;
    if (c == 'G')
    { /* possible GET */
        uint8_t rv = parse_request_get(client, line);
        PICO_FREE(line);
        line = NULL;
        return rv;
    }
    else if (c == 'P')
    { /* possible POST */
        uint8_t rv = parse_request_post(client, line);
        PICO_FREE(line);
        line = NULL;
        return rv;
    }
    PICO_FREE(line);
    line = NULL;
    return HTTP_RETURN_ERROR;
}

int16_t read_remaining_header(struct http_client *client)
{
    uint8_t *line = PICO_ZALLOC(1000u);
    if (!line)
        {
            pico_err = PICO_ERR_ENOMEM;
            return HTTP_RETURN_ERROR;
        }
    int32_t count = 0;
    int32_t len;

    while ((len = pico_socket_read(client->sck, line, 1000u)) > 0)
    {
        uint8_t c;
        int32_t index = 0;
        uint32_t body_len = 0;
        /* parse the response */
        while (index < len)
        {
            c = line[index++];
            if (c != '\r' && c != '\n')
                count++;

            if (c == '\n')
            {
                if (!count)
                {
                    client->state = HTTP_EOF_HDR;
                    /*dbg("End of header !\n");*/

                    body_len = (uint32_t)(len - index);
                    if (body_len > 0)
                    {
                        client->body = PICO_ZALLOC(body_len + 1u);
                        if (client->body)
                        {
                            memcpy(client->body, line + index, body_len);
                        }
                        else
                        {
                            PICO_FREE(line);
                            line = NULL;
                            return HTTP_RETURN_ERROR;
                        }
                    }

                    break;
                }

                count = 0;

            }
        }
    }
    PICO_FREE(line);
    line = NULL;
    return HTTP_RETURN_OK;
}

void send_data(struct http_client *client)
{
    uint16_t length;
    while (client->buffer_sent < client->buffer_size &&
           (length = (uint16_t)pico_socket_write(client->sck, (uint8_t *)client->buffer + client->buffer_sent, \
                                                 client->buffer_size - client->buffer_sent)) > 0 )
    {
        client->buffer_sent = (uint16_t)(client->buffer_sent + length);
        server.wakeup(EV_HTTP_PROGRESS, client->connectionID);
    }
    if (client->buffer_sent == client->buffer_size && client->buffer_size)
    {
        /* send chunk trail */
        if (pico_socket_write(client->sck, "\r\n", 2) > 0)
        {
            /* free the buffer */
            if (client->state == HTTP_SENDING_DATA)
            {
                PICO_FREE(client->buffer);
            }

            client->buffer = NULL;

            client->state = HTTP_WAIT_DATA;
            server.wakeup(EV_HTTP_SENT, client->connectionID);
        }
    }

}

void send_final(struct http_client *client)
{
    if (pico_socket_write(client->sck, "0\r\n\r\n", 5u) != 0)
    {
        pico_socket_close(client->sck);
        client->state = HTTP_CLOSED;
    }
    else
    {
        client->state = HTTP_SENDING_FINAL;
    }
}

int32_t read_data(struct http_client *client)
{
    if (!client)
    {
        dbg("Wrong connection ID\n");
        return HTTP_RETURN_ERROR;
    }

    if (client->state == HTTP_WAIT_HDR)
    {
        if (parse_request(client) < 0 || read_remaining_header(client) < 0)
        {
            return HTTP_RETURN_ERROR;
        }
    } /* continue with this in case the header comes line by line not a big chunk */
    else if (client->state == HTTP_WAIT_EOF_HDR)
    {
        if (read_remaining_header(client) < 0 )
            return HTTP_RETURN_ERROR;
    }

    if (client->state == HTTP_EOF_HDR)
    {
        client->state = HTTP_WAIT_RESPONSE;
        server.wakeup(EV_HTTP_REQ, client->connectionID);
    }

    return HTTP_RETURN_OK;
}

struct http_client *find_client(uint16_t conn)
{
    struct http_client dummy = {
        .connectionID = conn
    };

    return pico_tree_findKey(&pico_http_clients, &dummy);
}
