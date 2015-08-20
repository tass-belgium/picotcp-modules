/*********************************************************************
   PicoTCP. Copyright (c) 2012 TASS Belgium NV. Some rights reserved.
   See LICENSE and COPYING for usage.

   Author: Andrei Carp <andrei.carp@tass.be>
 *********************************************************************/
#include <string.h>
#include <stdint.h>
#include "pico_tree.h"
#include "pico_config.h"
#include "pico_socket.h"
#include "pico_tcp.h"
#include "pico_dns_client.h"
#include "pico_http_client.h"
#include "pico_http_util.h"
#include "pico_ipv4.h"
#include "pico_stack.h"

/*
 * This is the size of the following header
 *
 * GET <resource> HTTP/1.1<CRLF>
 * Host: <host>:<port><CRLF>
 * User-Agent: picoTCP<CRLF>
 * Connection: close<CRLF>
 * <CRLF>
 *
 * where <resource>,<host> and <port> will be added later.
 */

#define HTTP_GET_BASIC_SIZE   63u
#define HTTP_HEADER_LINE_SIZE 50u
#define RESPONSE_INDEX              9u

#define HTTP_CHUNK_ERROR    0xFFFFFFFFu

#ifdef dbg
    #undef dbg
#endif

#define dbg(...) do {} while(0)
#define nop() do {} while(0)

#define consume_char(c)                          (pico_socket_read(client->sck, &c, 1u))
#define is_location(line)                        (memcmp(line, "Location", 8u) == 0)
#define is_content_length(line)           (memcmp(line, "Content-Length", 14u) == 0u)
#define is_transfer_encoding(line)        (memcmp(line, "Transfer-Encoding", 17u) == 0u)
#define is_chunked(line)                         (memcmp(line, " chunked", 8u) == 0u)
#define is_not_HTTPv1(line)                       (memcmp(line, "HTTP/1.", 7u))
#define is_hex_digit(x) ((('0' <= x) && (x <= '9')) || (('a' <= x) && (x <= 'f')))
#define hex_digit_to_dec(x) ((('0' <= x) && (x <= '9')) ? (x - '0') : ((('a' <= x) && (x <= 'f')) ? (x - 'a' + 10) : (-1)))

static uint16_t global_client_conn_ID = 0;

struct pico_http_client
{
    uint16_t connectionID;
    uint8_t state;
    struct pico_socket *sck;
    void (*wakeup)(uint16_t ev, uint16_t conn);
    struct pico_ip4 ip;
    struct pico_http_uri *urikey;
    struct pico_http_header *header;
};

/* HTTP Client internal states */
#define HTTP_START_READING_HEADER      0
#define HTTP_READING_HEADER      1
#define HTTP_READING_BODY        2
#define HTTP_READING_CHUNK_VALUE 3
#define HTTP_READING_CHUNK_TRAIL 4

/* HTTP URI string parsing */
#define HTTP_PROTO_TOK      "http://"
#define HTTP_PROTO_LEN      7u

static int8_t pico_process_uri(const char *uri, struct pico_http_uri *urikey)
{

    uint16_t last_index = 0, index;

    if (!uri || !urikey || uri[0] == '/')
    {
        pico_err = PICO_ERR_EINVAL;
        goto error;
    }

    /* detect protocol => search for  "colon-slash-slash" */
    if (memcmp(uri, HTTP_PROTO_TOK, HTTP_PROTO_LEN) == 0) /* could be optimized */
    { /* protocol identified, it is http */
        urikey->protoHttp = 1;
        last_index = HTTP_PROTO_LEN;
    }
    else
    {
        if (strstr(uri, "://")) /* different protocol specified */
        {
            urikey->protoHttp = 0;
            goto error;
        }

        /* no protocol specified, assuming by default it's http */
        urikey->protoHttp = 1;
    }

    /* detect hostname */
    index = last_index;
    while (uri[index] && uri[index] != '/' && uri[index] != ':') index++;
    if (index == last_index)
    {
        /* wrong format */
        urikey->host = urikey->resource = NULL;
        urikey->port = urikey->protoHttp = 0u;
        pico_err = PICO_ERR_EINVAL;
        goto error;
    }
    else
    {
        /* extract host */
        urikey->host = (char *)PICO_ZALLOC((uint32_t)(index - last_index + 1));

        if(!urikey->host)
        {
            /* no memory */
            pico_err = PICO_ERR_ENOMEM;
            goto error;
        }

        memcpy(urikey->host, uri + last_index, (size_t)(index - last_index));
    }

    if (!uri[index])
    {
        /* nothing specified */
        urikey->port = 80u;
        urikey->resource = PICO_ZALLOC(2u);
        if (!urikey->resource) {
            /* no memory */
            pico_err = PICO_ERR_ENOMEM;
            goto error;
        }

        urikey->resource[0] = '/';
        return HTTP_RETURN_OK;
    }
    else if (uri[index] == '/')
    {
        urikey->port = 80u;
    }
    else if (uri[index] == ':')
    {
        urikey->port = 0u;
        index++;
        while (uri[index] && uri[index] != '/')
        {
            /* should check if every component is a digit */
            urikey->port = (uint16_t)(urikey->port * 10 + (uri[index] - '0'));
            index++;
        }
    }

    /* extract resource */
    if (!uri[index])
    {
        urikey->resource = PICO_ZALLOC(2u);
        if (!urikey->resource) {
            /* no memory */
            pico_err = PICO_ERR_ENOMEM;
            goto error;
        }

        urikey->resource[0] = '/';
    }
    else
    {
        last_index = index;
        while (uri[index] /*&& uri[index] != '?' && uri[index] != '&' && uri[index] != '#'*/) index++;
        urikey->resource = (char *)PICO_ZALLOC((size_t)(index - last_index + 1));

        if (!urikey->resource)
        {
            /* no memory */
            pico_err = PICO_ERR_ENOMEM;
            goto error;
        }

        memcpy(urikey->resource, uri + last_index, (size_t)(index - last_index));
    }

    return HTTP_RETURN_OK;

error:
    if (urikey->resource)
    {
        PICO_FREE(urikey->resource);
        urikey->resource = NULL;
    }

    if (urikey->host)
    {
        PICO_FREE(urikey->host);
        urikey->host = NULL;
    }

    return HTTP_RETURN_ERROR;
}

static int compare_clients(void *ka, void *kb)
{
    return ((struct pico_http_client *)ka)->connectionID - ((struct pico_http_client *)kb)->connectionID;
}

PICO_TREE_DECLARE(pico_client_list, compare_clients);

/* Local functions */
static int parse_header_from_server(struct pico_http_client *client, struct pico_http_header *header);
static int read_chunk_line(struct pico_http_client *client);
/*  */
static inline void process_conn_err_close(uint16_t ev, struct pico_http_client *client)
{
    if (!client)
        return;

    if (ev & PICO_SOCK_EV_CONN)
        client->wakeup(EV_HTTP_CON, client->connectionID);

    if (ev & PICO_SOCK_EV_ERR)
    {
        client->wakeup(EV_HTTP_ERROR, client->connectionID);
    }

    if ((ev & PICO_SOCK_EV_CLOSE) || (ev & PICO_SOCK_EV_FIN))
    {
        client->wakeup(EV_HTTP_CLOSE, client->connectionID);
    }
}

static inline void wait_for_header(struct pico_http_client *client)
{
    /* wait for header */
    int http_ret;

    http_ret = parse_header_from_server(client, client->header);
    if (http_ret < 0)
    {
        client->wakeup(EV_HTTP_ERROR, client->connectionID);
    }
    else if (http_ret == HTTP_RETURN_BUSY)
    {
        client->state = HTTP_READING_HEADER;
    }
    else if (http_ret == HTTP_RETURN_NOT_FOUND)
    {
        client->wakeup(EV_HTTP_REQ, client->connectionID);
    }
    else
    {
        /* call wakeup */
        if (client->header->response_code != HTTP_CONTINUE)
        {
            client->wakeup(
                (client->header->response_code == HTTP_OK) ?
                (EV_HTTP_REQ | EV_HTTP_BODY) :     /* data comes for sure only when 200 is received */
                EV_HTTP_REQ
                , client->connectionID);
        }
    }
}

static inline void treat_read_event(struct pico_http_client *client)
{
    /* read the header, if not read */
    dbg("treat read event, client state: %d\n", client->state);
    if (client->state == HTTP_START_READING_HEADER)
    {
        /* wait for header */
        client->header = PICO_ZALLOC(sizeof(struct pico_http_header));
        if (!client->header)
        {
            pico_err = PICO_ERR_ENOMEM;
            return;
        }

        wait_for_header(client);
    }
    else if (client->state == HTTP_READING_HEADER)
    {
        wait_for_header(client);
    }
    else
    {
        /* just let the user know that data has arrived, if chunked data comes, will be treated in the */
        /* read api. */
        client->wakeup(EV_HTTP_BODY, client->connectionID);
    }
}

static void tcp_callback(uint16_t ev, struct pico_socket *s)
{

    struct pico_http_client *client = NULL;
    struct pico_tree_node *index;

    dbg("tcp callback (%d)\n", ev);

    /* find http_client */
    pico_tree_foreach(index, &pico_client_list)
    {
        if (((struct pico_http_client *)index->keyValue)->sck == s )
        {
            client = (struct pico_http_client *)index->keyValue;
            break;
        }
    }

    if (!client)
    {
        dbg("Client not found...Something went wrong !\n");
        return;
    }

    process_conn_err_close(ev, client);

    if (ev & PICO_SOCK_EV_RD)
    {
        treat_read_event(client);
    }
}

/* used for getting a response from DNS servers */
static void dns_callback(char *ip, void *ptr)
{
    struct pico_http_client *client = (struct pico_http_client *)ptr;

    if (!client)
    {
        dbg("Who made the request ?!\n");
        return;
    }

    if (ip)
    {
        client->wakeup(EV_HTTP_DNS, client->connectionID);

        /* add the ip address to the client, and start a tcp connection socket */
        pico_string_to_ipv4(ip, &client->ip.addr);
        client->sck = pico_socket_open(PICO_PROTO_IPV4, PICO_PROTO_TCP, &tcp_callback);
        if (!client->sck)
        {
            client->wakeup(EV_HTTP_ERROR, client->connectionID);
            return;
        }

        if (pico_socket_connect(client->sck, &client->ip, short_be(client->urikey->port)) < 0)
        {
            client->wakeup(EV_HTTP_ERROR, client->connectionID);
            return;
        }

    }
    else
    {
        /* wakeup client and let know error occured */
        client->wakeup(EV_HTTP_ERROR, client->connectionID);

        /* close the client (free used heap) */
        pico_http_client_close(client->connectionID);
    }
}

/*
 * API used for opening a new HTTP Client.
 *
 * The accepted uri's are [http:]hostname[:port]/resource
 * no relative uri's are accepted.
 *
 * The function returns a connection ID >= 0 if successful
 * -1 if an error occured.
 */
int MOCKABLE pico_http_client_open(char *uri, void (*wakeup)(uint16_t ev, uint16_t conn))
{
    struct pico_http_client *client;
    uint32_t ip = 0;

    client = PICO_ZALLOC(sizeof(struct pico_http_client));
    if(!client)
    {
        /* memory error */
        pico_err = PICO_ERR_ENOMEM;
        return HTTP_RETURN_ERROR;
    }

    client->wakeup = wakeup;
    client->connectionID = global_client_conn_ID++;

    client->urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));

    if (!client->urikey)
    {
        pico_err = PICO_ERR_ENOMEM;
        PICO_FREE(client);
        return HTTP_RETURN_ERROR;
    }

    pico_process_uri(uri, client->urikey);

    if (pico_tree_insert(&pico_client_list, client))
    {
        /* already in */
        pico_err = PICO_ERR_EEXIST;
        PICO_FREE(client->urikey);
        PICO_FREE(client);
        return HTTP_RETURN_ALREADYIN;
    }

    /* dns query */
    if (pico_string_to_ipv4(client->urikey->host, &ip) == -1)
    {
        dbg("Querying : %s \n", client->urikey->host);
        pico_dns_client_getaddr(client->urikey->host, dns_callback, client);
    }
    else
    {
        dbg("host already and ip address, no dns required");
        dns_callback(client->urikey->host, client);
    }

    /* return the connection ID */
    return client->connectionID;
}

/*
 * API for sending a header to the client.
 *
 * if hdr == HTTP_HEADER_RAW , then the parameter header
 * is sent as it is to client.
 *
 * if hdr == HTTP_HEADER_DEFAULT, then the parameter header
 * is ignored and the library will build the response header
 * based on the uri passed when opening the client.
 *
 */
int32_t MOCKABLE pico_http_client_send_header(uint16_t conn, char *header, uint8_t hdr)
{
    struct pico_http_client search = {
        .connectionID = conn
    };
    struct pico_http_client *http = pico_tree_findKey(&pico_client_list, &search);
    int32_t length;
    if (!http)
    {
        dbg("Client not found !\n");
        return HTTP_RETURN_ERROR;
    }

    /* the api gives the possibility to the user to build the GET header */
    /* based on the uri passed when opening the client, less headache for the user */
    if (hdr == HTTP_HEADER_DEFAULT)
    {
        header = pico_http_client_build_header(http->urikey);

        if (!header)
        {
            pico_err = PICO_ERR_ENOMEM;
            return HTTP_RETURN_ERROR;
        }
    }

    length = pico_socket_write(http->sck, (void *)header, (int)strlen(header));

    if (hdr == HTTP_HEADER_DEFAULT)
        PICO_FREE(header);

    return length;
}


/* / */

static inline int check_chunk_line(struct pico_http_client *client, int tmp_len_read)
{
    if (read_chunk_line(client) == HTTP_RETURN_ERROR)
    {
        dbg("Probably the chunk is malformed or parsed wrong...\n");
        client->wakeup(EV_HTTP_ERROR, client->connectionID);
        return HTTP_RETURN_ERROR;
    }

    if (client->state != HTTP_READING_BODY || !tmp_len_read)
        return 0; /* force out */

    return 1;
}

static inline void update_content_length(struct pico_http_client *client, int tmp_len_read )
{
    if (tmp_len_read > 0)
    {
        client->header->content_length_or_chunk = client->header->content_length_or_chunk - (uint32_t)tmp_len_read;
    }
}

static inline int read_body(struct pico_http_client *client, char *data, uint16_t size, int *len_read, int *tmp_len_read)
{
    *tmp_len_read = 0;

    if (client->state == HTTP_READING_BODY)
    {

        /* if needed truncate the data */
        *tmp_len_read = pico_socket_read(client->sck, data + (*len_read),
                                       (client->header->content_length_or_chunk < ((uint32_t)(size - (*len_read)))) ? ((int)client->header->content_length_or_chunk) : (size - (*len_read)));

        update_content_length(client, *tmp_len_read);
        if (*tmp_len_read < 0)
        {
            /* error on reading */
            dbg(">>> Error returned pico_socket_read\n");
            pico_err = PICO_ERR_EBUSY;
            /* return how much data was read until now */
            return (*len_read);
        }
    }

    *len_read += *tmp_len_read;
    return 0;
}

static inline int read_big_chunk(struct pico_http_client *client, char *data, uint16_t size, int *len_read)
{
    int value;
    /* check if we need more than one chunk */
    if (size >= client->header->content_length_or_chunk)
    {
        /* read the rest of the chunk, if chunk is done, proceed to the next chunk */
        while ((uint16_t)(*len_read) <= size)
        {
            int tmp_len_read = 0;
            if (read_body(client, data, size, len_read, &tmp_len_read))
                return (*len_read);

            if ((value = check_chunk_line(client, tmp_len_read)) <= 0)
                return value;
        }
    }

    return 0;
}

static inline void read_small_chunk(struct pico_http_client *client, char *data, uint16_t size, int *len_read)
{
    if (size < client->header->content_length_or_chunk)
    {
        /* read the data from the chunk */
        *len_read = pico_socket_read(client->sck, (void *)data, size);

        if (*len_read)
            client->header->content_length_or_chunk = client->header->content_length_or_chunk - (uint32_t)(*len_read);
    }
}
static inline int read_chunked_data(struct pico_http_client *client, char *data, uint16_t size)
{
    int len_read = 0;
    int value;
    /* read the chunk line */
    if (read_chunk_line(client) == HTTP_RETURN_ERROR)
    {
        dbg("Probably the chunk is malformed or parsed wrong...\n");
        client->wakeup(EV_HTTP_ERROR, client->connectionID);
        return HTTP_RETURN_ERROR;
    }

    /* nothing to read, no use to try */
    if (client->state != HTTP_READING_BODY)
    {
        pico_err = PICO_ERR_EAGAIN;
        return HTTP_RETURN_OK;
    }


    read_small_chunk(client, data, size, &len_read);
    value = read_big_chunk(client, data, size, &len_read);
    if (value)
        return value;

    return len_read;
}

/*
 * API for reading received data.
 *
 * This api hides from the user if the transfer-encoding
 * was chunked or a full length was provided, in case of
 * a chunked transfer encoding will "de-chunk" the data
 * and pass it to the user.
 */
int32_t MOCKABLE pico_http_client_read_data(uint16_t conn, char *data, uint16_t size)
{
    struct pico_http_client dummy = {
        .connectionID = conn
    };
    struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &dummy);

    if (!client)
    {
        dbg("Wrong connection id !\n");
        pico_err = PICO_ERR_EINVAL;
        return HTTP_RETURN_ERROR;
    }

    /* for the moment just read the data, do not care if it's chunked or not */
    if (client->header->transfer_coding == HTTP_TRANSFER_FULL)
        return pico_socket_read(client->sck, (void *)data, size);
    else
        return read_chunked_data(client, data, size);
}

/*
 * API for reading received data.
 *
 * Reads out the header struct received from server.
 */
struct pico_http_header * MOCKABLE pico_http_client_read_header(uint16_t conn)
{
    struct pico_http_client dummy = {
        .connectionID = conn
    };
    struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &dummy);

    if (client)
    {
        return client->header;
    }
    else
    {
        /* not found */
        dbg("Wrong connection id !\n");
        pico_err = PICO_ERR_EINVAL;
        return NULL;
    }
}

/*
 * API for reading received data.
 *
 * Reads out the uri struct after was processed.
 */
struct pico_http_uri *pico_http_client_read_uri_data(uint16_t conn)
{
    struct pico_http_client dummy = {
        .connectionID = conn
    };
    struct pico_http_client *client = pico_tree_findKey(&pico_client_list, &dummy);
    /*  */
    if (client)
        return client->urikey;
    else
    {
        /* not found */
        dbg("Wrong connection id !\n");
        pico_err = PICO_ERR_EINVAL;
        return NULL;
    }
}

/*
 * API for reading received data.
 *
 * Close the client.
 */
static inline void free_header(struct pico_http_client *to_be_removed)
{
    if (to_be_removed->header)
    {
        /* free space used */
        if (to_be_removed->header->location)
            PICO_FREE(to_be_removed->header->location);

        PICO_FREE(to_be_removed->header);
    }
}

static inline void free_uri(struct pico_http_client *to_be_removed)
{
    if (to_be_removed->urikey)
    {
        if (to_be_removed->urikey->host)
            PICO_FREE(to_be_removed->urikey->host);

        if (to_be_removed->urikey->resource)
            PICO_FREE(to_be_removed->urikey->resource);

        PICO_FREE(to_be_removed->urikey);
    }
}
int MOCKABLE pico_http_client_close(uint16_t conn)
{
    struct pico_http_client *to_be_removed = NULL;
    struct pico_http_client dummy = {
        0
    };
    dummy.connectionID = conn;

    dbg("Closing the client...\n");
    to_be_removed = pico_tree_delete(&pico_client_list, &dummy);
    if (!to_be_removed)
    {
        dbg("Warning ! Element not found ...");
        return HTTP_RETURN_ERROR;
    }

    /* close socket */
    if (to_be_removed->sck)
        pico_socket_close(to_be_removed->sck);

    free_header(to_be_removed);
    free_uri(to_be_removed);

    PICO_FREE(to_be_removed);

    return 0;
}

/*
 * API for reading received data.
 *
 * Builds a GET header based on the fields on the uri.
 */
char *pico_http_client_build_header(const struct pico_http_uri *uri_data)
{
    char *header;
    char port[6u]; /* 6 = max length of a uint16 + \0 */

    unsigned long header_size = HTTP_GET_BASIC_SIZE;

    if (!uri_data->host || !uri_data->resource || !uri_data->port)
    {
        pico_err = PICO_ERR_EINVAL;
        return NULL;
    }

    /*  */
    header_size = (header_size + strlen(uri_data->host));
    header_size = (header_size + strlen(uri_data->resource));
    header_size = (header_size + pico_itoa(uri_data->port, port) + 4u); /* 3 = size(CRLF + \0) */
    header = PICO_ZALLOC(header_size);

    if (!header)
    {
        /* not enought memory */
        pico_err = PICO_ERR_ENOMEM;
        return NULL;
    }

    /* build the actual header */
    strcpy(header, "GET ");
    strcat(header, uri_data->resource);
    strcat(header, " HTTP/1.1\r\n");
    strcat(header, "Host: ");
    strcat(header, uri_data->host);
    strcat(header, ":");
    strcat(header, port);
    strcat(header, "\r\n");
    strcat(header, "User-Agent: picoTCP\r\nConnection: close\r\n\r\n"); /* ? */

    return header;
}


/*  */
static inline void read_first_line(struct pico_http_client *client, char *line, uint32_t *index)
{
    char c;

    /* read the first line of the header */
    while (consume_char(c) > 0 && c != '\r')
    {
        if (*index < HTTP_HEADER_LINE_SIZE) /* truncate if too long */
            line[(*index)++] = c;
    }
    consume_char(c); /* consume \n */
}

static inline void start_reading_body(struct pico_http_client *client, struct pico_http_header *header)
{

    if (header->transfer_coding == HTTP_TRANSFER_CHUNKED)
    {
        /* read the first chunk */
        header->content_length_or_chunk = 0;

        client->state = HTTP_READING_CHUNK_VALUE;
        read_chunk_line(client);

    }
    else
        client->state = HTTP_READING_BODY;
}

static inline int parse_loc_and_cont(struct pico_http_client *client, struct pico_http_header *header, char *line, uint32_t *index)
{
    char c;
    /* Location: */

    if (is_location(line))
    {
        *index = 0;
        while (consume_char(c) > 0 && c != '\r')
        {
            line[(*index)++] = c;
        }
        /* allocate space for the field */
        header->location = PICO_ZALLOC((*index) + 1u);
        if (header->location)
        {
            memcpy(header->location, line, (*index));
            return 1;
        }
        else
        {
            return -1;
        }
    }    /* Content-Length: */
    else if (is_content_length(line))
    {
        header->content_length_or_chunk = 0u;
        header->transfer_coding = HTTP_TRANSFER_FULL;
        /* consume the first space */
        consume_char(c);
        while (consume_char(c) > 0 && c != '\r')
        {
            header->content_length_or_chunk = header->content_length_or_chunk * 10u + (uint32_t)(c - '0');
        }
        return 1;
    }    /* Transfer-Encoding: chunked */

    return 0;
}

static inline int parse_transfer_encoding(struct pico_http_client *client, struct pico_http_header *header, char *line, uint32_t *index)
{
    char c;

    if (is_transfer_encoding(line))
    {
        (*index) = 0;
        while (consume_char(c) > 0 && c != '\r')
        {
            line[(*index)++] = c;
        }
        if (is_chunked(line))
        {
            header->content_length_or_chunk = 0u;
            header->transfer_coding = HTTP_TRANSFER_CHUNKED;
        }

        return 1;
    } /* just ignore the line */

    return 0;
}


static inline int parse_fields(struct pico_http_client *client, struct pico_http_header *header, char *line, uint32_t *index)
{
    char c;
    int ret_val;

    ret_val = parse_loc_and_cont(client, header, line, index);
    if (ret_val == 0)
    {
        if (!parse_transfer_encoding(client, header, line, index))
        {
            while (consume_char(c) > 0 && c != '\r') nop();
        }
    }
    else if (ret_val == -1)
    {
        return -1;
    }

    /* consume the next one */
    consume_char(c);
    /* reset the index */
    (*index) = 0u;

    return 0;
}

static inline int parse_rest_of_header(struct pico_http_client *client, struct pico_http_header *header, char *line, uint32_t *index)
{
    char c;
    int read_len = 0;

    /* parse the rest of the header */
    read_len = consume_char(c);
    if (read_len == 0)
        return HTTP_RETURN_BUSY;

    while (read_len > 0)
    {
        if (c == ':')
        {
            if (parse_fields(client, header, line, index) == -1)
                return HTTP_RETURN_ERROR;
        }
        else if (c == '\r' && !(*index))
        {
            /* consume the \n */
            consume_char(c);
            break;
        }
        else
        {
            line[(*index)++] = c;
        }

        read_len = consume_char(c);
    }
    return HTTP_RETURN_OK;
}

static int parse_header_from_server(struct pico_http_client *client, struct pico_http_header *header)
{
    char line[HTTP_HEADER_LINE_SIZE];
    uint32_t index = 0;

    if (client->state == HTTP_START_READING_HEADER)
    {
        read_first_line(client, line, &index);
        /* check the integrity of the response */
        /* make sure we have enough characters to include the response code */
        /* make sure the server response starts with HTTP/1. */
        if ((index < RESPONSE_INDEX + 2u) || is_not_HTTPv1(line))
        {
            /* wrong format of the the response */
            pico_err = PICO_ERR_EINVAL;
            return HTTP_RETURN_ERROR;
        }

        /* extract response code */
        header->response_code = (uint16_t)((line[RESPONSE_INDEX] - '0') * 100 +
                                          (line[RESPONSE_INDEX + 1] - '0') * 10 +
                                          (line[RESPONSE_INDEX + 2] - '0'));
        if (header->response_code == HTTP_NOT_FOUND)
        {
            return HTTP_RETURN_NOT_FOUND;
        }
        else if (header->response_code >= HTTP_INTERNAL_SERVER_ERR)
        {
            /* invalid response type */
            header->response_code = 0;
            return HTTP_RETURN_ERROR;
        }
    }

    dbg("Server response : %d \n", header->response_code);

    if (parse_rest_of_header(client, header, line, &index) == HTTP_RETURN_BUSY)
        return HTTP_RETURN_BUSY;

    start_reading_body(client, header);
    dbg("End of header\n");
    return HTTP_RETURN_OK;

}

/* an async read of the chunk part, since in theory a chunk can be split in 2 packets */
static inline void set_client_chunk_state(struct pico_http_client *client)
{

    if (client->header->content_length_or_chunk == 0 && client->state == HTTP_READING_BODY)
    {
        client->state = HTTP_READING_CHUNK_VALUE;
    }
}
static inline void read_chunk_trail(struct pico_http_client *client)
{
    char c;

    if (client->state == HTTP_READING_CHUNK_TRAIL)
    {

        while (consume_char(c) > 0 && c != '\n') nop();
        if (c == '\n') client->state = HTTP_READING_BODY;
    }
}
static inline void read_chunk_value(struct pico_http_client *client)
{
    char c;

    while (consume_char(c) > 0 && c != '\r' && c != ';')
    {
        if (is_hex_digit(c))
            client->header->content_length_or_chunk = (client->header->content_length_or_chunk << 4u) + (uint32_t)hex_digit_to_dec(c);
    }
    if (c == '\r' || c == ';') client->state = HTTP_READING_CHUNK_TRAIL;
}

static int read_chunk_line(struct pico_http_client *client)
{
    set_client_chunk_state(client);

    if (client->state == HTTP_READING_CHUNK_VALUE)
    {
        read_chunk_value(client);
    }

    read_chunk_trail(client);

    return HTTP_RETURN_OK;
}
