#include "pico_websocket_client.h"
#include "../libhttp/pico_http_util.h"
#include <stdint.h>
#include <string.h>
#include "pico_tree.h"
#include "pico_config.h"
#include "pico_socket.h"
#include "pico_tcp.h"
#include "pico_ipv4.h"
#include "pico_stack.h"
#include "pico_socket.h"
#include "pico_bsd_sockets.h"

#ifdef SSL_WEBSOCKET
#include "wolfssl/ssl.h"
extern unsigned char *ssl_cert_pem;
extern unsigned int *ssl_cert_pem_len;
#define backend_read wolfSSL_read
#define backend_write wolfSSL_write
#else
#define backend_read pico_read
#define backend_write pico_write 
#endif

#define WS_PROTO_TOK                           "ws://"
#define WS_PROTO_LEN                           5u

#define HTTP_HEADER_LINE_SIZE                  50u
#define HTTP_RESPONSE_CODE_INDEX               9u
#define WEBSOCKET_COMMON_HEADER_SIZE           (sizeof(struct pico_websocket_header))

#define PAYLOAD_LENGTH_SIZE_16_BIT_IN_BYTES    2u
#define PAYLOAD_LENGTH_SIZE_64_BIT_IN_BYTES    8u

#define WS_MASKING_KEY_SIZE_IN_BYTES           4u

#define WS_BUFFER_SIZE                         5*1024u /* Provide 5K for fragmented messages (recv or sending) */
#define WS_FRAME_SIZE                          1024u   /* We define one frame as 1K payload */

#define WS_CONTROL_FRAME_MAX_SIZE              125u

#define WS_CLOSING_WAIT_TIME_IN_MS             5000u

#define WS_STATUS_CODE_SIZE_IN_BYTES           2u
#define WS_DEFAULT_STATUS_CODE                 1005u

/* States of the websocket client */
#define WS_CONNECTING                          1u
#define WS_CONNECTED                           2u
#define WS_CLOSING                             3u
/* I sent a close frame and am waiting for the server to sent his. If WS_CLOSE_PERIOD expires before this happening, I will close the tcp connection */
#define WS_CLOSED                              4u


/* 
 * client could be recv fragmented message and sending fragmented message at the same time
 */
struct pico_websocket_client
{
        int fd;
        struct pico_http_uri uriKey;
        /* Frag support */
        void* buffer;
        void* next_free_pos_in_buffer;
        struct sockaddr_in addr;
        char* protocol;
        char* extensions;
        char* extension_arguments;
        int state;
        void *ssl_ctx;
        void *ssl;
};

PACKED_STRUCT_DEF pico_websocket_header
{
        uint8_t opcode : 4;
        uint8_t RSV3 : 1;
        uint8_t RSV2 : 1;
        uint8_t RSV1 : 1;
        uint8_t fin : 1;
        uint8_t payload_length : 7;
        uint8_t mask : 1;
};

/* This struct is used when sending data to the server, the user sets the RSV bits with the function pico_websocket_client_set_RSV_bits() */
struct pico_websocket_RSV
{
        uint8_t RSV3 : 1;
        uint8_t RSV2 : 1;
        uint8_t RSV1 : 1;
};


static void cleanup_websocket_client(pico_time now, void* args);
static int pico_websocket_mask_data(uint32_t masking_key, uint8_t* data, int size);

/* TODO: move to pico_http_util */
static int8_t pico_process_URI(const char *uri, struct pico_http_uri *urikey)
{

        uint16_t lastIndex = 0, index;

        if(!uri || !urikey || uri[0] == '/')
        {
                pico_err = PICO_ERR_EINVAL;
                goto error;
        }

/* detect protocol => search for  "colon-slash-slash" */
        if(memcmp(uri, WS_PROTO_TOK, WS_PROTO_LEN) == 0) /* could be optimized */
        { /* protocol identified, it is ws */
                urikey->protoHttp = 1;
                lastIndex = WS_PROTO_LEN;
        }

/* detect hostname */
        index = lastIndex;
        while(uri[index] && uri[index] != '/' && uri[index] != ':') index++;
        if(index == lastIndex)
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
                urikey->host = (char *)PICO_ZALLOC((uint32_t)(index - lastIndex + 1));

                if(!urikey->host)
                {
/* no memory */
                        pico_err = PICO_ERR_ENOMEM;
                        goto error;
                }

                memcpy(urikey->host, uri + lastIndex, (size_t)(index - lastIndex));
        }

        if(!uri[index])
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
                return 0;
        }
        else if(uri[index] == '/')
        {
                urikey->port = 80u;
        }
        else if(uri[index] == ':')
        {
                urikey->port = 0u;
                index++;
                while(uri[index] && uri[index] != '/')
                {
/* should check if every component is a digit */
                        urikey->port = (uint16_t)(urikey->port * 10 + (uri[index] - '0'));
                        index++;
                }
        }

/* extract resource */
        if(!uri[index])
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
                lastIndex = index;
                while(uri[index]) index++;
                urikey->resource = (char *)PICO_ZALLOC((size_t)(index - lastIndex + 1));

                if(!urikey->resource)
                {
/* no memory */
                        pico_err = PICO_ERR_ENOMEM;
                        goto error;
                }

                memcpy(urikey->resource, uri + lastIndex, (size_t)(index - lastIndex));
        }

        return 0;

error:
        if(urikey->resource)
        {
                PICO_FREE(urikey->resource);
                urikey->resource = NULL;
        }

        if(urikey->host)
        {
                PICO_FREE(urikey->host);
                urikey->host = NULL;
        }

        return -1;
}

/* TODO: this function is copied from pico_http_util.c, make http_util.c a common library or something to avoid code duplication */
uint32_t pico_itoa(uint32_t port, char *ptr)
{
        uint32_t size = 0;
        uint32_t index;

        /* transform to from number to string [ in backwards ] */
        while(port)
        {
                ptr[size] = (char)(port % 10 + '0');
                port = port / 10;
                size++;
        }
        /* invert positions */
        for(index = 0; index < (size >> 1u); index++)
        {
                char c = ptr[index];
                ptr[index] = ptr[size - index - 1];
                ptr[size - index - 1] = c;
        }
        ptr[size] = '\0';
        return size;
}

/* If close frame has a payload, the first two bytes in the payload are an unisgned int in network order. This is why we add WS_STATUS_CODE_SIZE_IN_BYTES to the reason_size when checking the size. */
static int send_close_frame(struct pico_websocket_client* client, uint16_t status_code, void* reason, uint8_t reason_size)
{
        int ret;
        uint32_t masking_key;
        struct pico_websocket_header* hdr = PICO_ZALLOC(sizeof(struct pico_websocket_header));

        hdr->opcode = WS_CONN_CLOSE;
        hdr->fin = WS_FIN_ENABLE;
        hdr->RSV1 = 0;
        hdr->RSV2 = 0;
        hdr->RSV3 = 0;
        hdr->mask = WS_MASK_ENABLE;
        masking_key = pico_rand();

        if (reason_size + WS_STATUS_CODE_SIZE_IN_BYTES > WS_CONTROL_FRAME_MAX_SIZE)
        {
                dbg("Size of reason is too big to fit in close frame.\n");
                return -1;
        }

        if (reason && reason_size)
        {
                hdr->payload_length = reason_size + WS_STATUS_CODE_SIZE_IN_BYTES;
        }

        ret = backend_write(client->fd, hdr, sizeof(struct pico_websocket_header));

        if (ret < 0)
        {
                dbg("Failed to write header for close frame.\n");
                return -1;
        }

        ret = backend_write(client->fd, &masking_key, WS_MASKING_KEY_SIZE_IN_BYTES);

        if (ret < 0)
        {
                dbg("Failed to write header for close frame.\n");
                return -1;
        }

        if (reason && reason_size)
        {
                /* When no status code is provided, the server will default to 1005, so not our responsibility */
                if (status_code)
                {
                        status_code = short_be(status_code);
                        status_code = pico_websocket_mask_data(masking_key, (uint8_t *)&status_code, WS_STATUS_CODE_SIZE_IN_BYTES );
                        ret = backend_write(client->fd, &status_code, WS_STATUS_CODE_SIZE_IN_BYTES);
                }
                pico_websocket_mask_data(masking_key, reason, reason_size);
                ret = backend_write(client->fd, reason, reason_size);
        }


        /* client->state = WS_CLOSING; */

        return ret;
}


static int fail_websocket_connection(struct pico_websocket_client* client)
{
        switch (client->state)
        {
        case WS_CONNECTED:
                /* I recv a close frame, I should sent one back. Now I am closed*/
                send_close_frame(client, 0, NULL, 0);
                client->state = WS_CLOSED;
                pico_timer_add(WS_CLOSING_WAIT_TIME_IN_MS, cleanup_websocket_client, (void*)client);
                break;
        case WS_CLOSING:
                /* I sent a close frame and am waiting for the servers response */
                pico_timer_add(WS_CLOSING_WAIT_TIME_IN_MS, cleanup_websocket_client, (void*)client);
                break;
        case WS_CLOSED:
                break;
        default:
                pico_timer_add(WS_CLOSING_WAIT_TIME_IN_MS, cleanup_websocket_client, (void*)client);
                break;
        }

        return 0;
}



static int pico_websocket_client_cleanup(struct pico_websocket_client* client)
{
        dbg("Closing the websocket client...\n");

        if (client->fd > 0) {
            pico_close(client->fd);
            client->fd = -1;
        }

        if (client->buffer)
        {
                PICO_FREE(client->buffer);
        }
#ifdef SSL_WEBSOCKET
        if (client->ssl)
            PICO_FREE(client->ssl);
        if (client->ssl_ctx)
            PICO_FREE(client->ssl_ctx);
#endif

        PICO_FREE(client);

        dbg("Client was succesfully closed.\n");
        return 0;
}

static void cleanup_websocket_client(pico_time now, void* args)
{
        struct pico_websocket_client* client = (struct pico_websocket_client*) args;

        pico_websocket_client_cleanup(client);
}


static int pico_websocket_mask_data(uint32_t masking_key, uint8_t* data, int size)
{
        int i;
        uint8_t mask[4];

        memcpy(mask, &masking_key, WS_MASKING_KEY_SIZE_IN_BYTES);

        for( i = 0 ; i < size ; ++i )
        {
                data[i] = data[i] ^ mask[i%4];
        }

        return i;
}

static int read_char_from_client_socket(WSocket ws, char *c)
{
    return backend_read(ws->fd, c, 1);
}

/* Note: "\r\n" is not considered a line, so the last line of a http message will give back strlen(line) == 0 */
static void ws_read_http_header_line(struct pico_websocket_client* client, char *line)
{
        char c;
        uint32_t index = 0;

        while(read_char_from_client_socket(client, &c) > 0 && c != '\r')
        {
                line[index++] = c;
        }
        line[index] = (char)0;
        read_char_from_client_socket(client, &c); /* Consume '\n' */
}

static void add_protocols_to_header(char* header, char* protocol)
{
        if (!protocol)
        {
                /* No protocols specified by user, defaulting to chat and superchat */
                strcat(header, "Sec-WebSocket-Protocol: chat, superchat\r\n");
                return;
        }
        else
        {

                strcat(header, "Sec-WebSocket-Protocol: ");
                strcat(header, protocol);
        }
        strcat(header, "\r\n");
}

static void add_extensions_to_header(char* header, char* extensions)
{
        if (!extensions)
        {
                /* No extensions specified by user, defaulting to no extensions */
                return;
        } else {
                strcat(header, "Sec-WebSocket-Extensions: ");
                strcat(header, extensions);
        }
        strcat(header, "\r\n");
}


static char* build_pico_websocket_upgradeHeader(struct pico_websocket_client* client)
{
        /* TODO: review this building process */
        char* header;
        int header_size = 256;

        header = PICO_ZALLOC(header_size);
        strcpy(header, "GET /chat HTTP/1.1\r\n");
        strcat(header, "Host: ");
        strcat(header, client->uriKey.host);

        if (client->uriKey.port)
        {
                char port[6u];
                pico_itoa(client->uriKey.port, port);
                strcat(header, ":");
                strcat(header, port);
        }
        strcat(header, "\r\n");
        strcat(header, "Upgrade: websocket\r\n");
        strcat(header, "Connection: Upgrade\r\n");
        strcat(header, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"); /*TODO: this is a 16-byte value that has been base64-encoded, it is randomly selected. */

        add_protocols_to_header(header, client->protocol);
        add_extensions_to_header(header, client->extensions);
        strcat(header, "Sec-WebSocket-Version: 13\r\n");
        strcat(header, "\r\n");
        return header;
}

static int pico_websocket_client_send_upgrade_header(struct pico_websocket_client* client)
{
        char *header;
        int ret;

        header = build_pico_websocket_upgradeHeader(client);
        if (!header)
        {
                dbg("WebSocket Header could not be created.\n");
                return -1;
        }

        ret = backend_write(client->fd, header, strlen(header));

        if(ret < 0)
        {
                dbg("Failed to send upgrade header.\n");
                return -1;
        }

        return ret;
}


static int parse_server_http_respone_line(char* line, char* colon_ptr)
{
        /* TODO */
        /* Don't forgot to check for the pico_websocket_client_handshake options */
        return 0;
}

static int ws_parse_upgrade_header(struct pico_websocket_client* client)
{
        char* line = PICO_ZALLOC(HTTP_HEADER_LINE_SIZE);
        char* colon_ptr;
        uint16_t responseCode = -1;
        int ret;

        ws_read_http_header_line(client, line);

        responseCode = (uint16_t)((line[HTTP_RESPONSE_CODE_INDEX] - '0') * 100 +
                                  (line[HTTP_RESPONSE_CODE_INDEX + 1] - '0') * 10 +
                                  (line[HTTP_RESPONSE_CODE_INDEX + 2] - '0'));

        switch (responseCode)
        {
        case HTTP_SWITCHING_PROTOCOLS:
                dbg("Response to upgrade request was HTTP_SWITCHING_PROTOCOLS (101).\n");
                break;
        case HTTP_UNAUTH:
                /* TODO */
                return -1;
        default:
                if (responseCode >= HTTP_MULTI_CHOICE && responseCode < HTTP_BAD_REQUEST)
                {
                        /* REDIRECTION */
                        /* TODO */
                        return -1;
                }
                return -1;
        }

        /* Response code was OK, validate rest of the response */
        do{
                ws_read_http_header_line(client, line);
                colon_ptr = strstr(line, ":");
                if (colon_ptr != NULL)
                {
                        ret = parse_server_http_respone_line(line, colon_ptr);
                        if (ret < 0)
                        {
                                /* TODO: cancel everything */
                        }
                }
        } while ( strlen(line) != 0 );

        return 0;
}

static int handle_ctrl_packet(WSocket ws, struct pico_websocket_header *hdr, int payload_length)
{
    if (payload_length > WS_CONTROL_FRAME_MAX_SIZE) {
        ws_close(ws);
        pico_err = PICO_ERR_ECONNRESET;
        return -1;
    }
    if (hdr->opcode == WS_CONN_CLOSE) {
        ws_close(ws);
        pico_err = PICO_ERR_ENOTCONN;
        return -1;
    }
    if (hdr->opcode == WS_PONG) {
        /* XXX Implement ping-pong! */
        return 0;
    }
    ws_close(ws);
    return -1;
}

static void pico_websocket_client_send_pong(WSocket ws, struct pico_websocket_header *hdr, void *data, int size)
{
    uint32_t masking_key = pico_rand();
    hdr->opcode = WS_PONG;
    hdr->mask = WS_MASK_ENABLE;
    backend_write(ws->fd, hdr, sizeof(struct pico_websocket_header));
    backend_write(ws->fd, &masking_key, sizeof(uint32_t));
    if (size > 0) {
        pico_websocket_mask_data(masking_key, data, size);
        backend_write(ws->fd, data, size);
        pico_websocket_mask_data(masking_key, data, size); /* Unmask data in place, that's going back to user! */
    }
}

static WSocket build_pico_websocket_client(void)
{
        /* TODO: solve memory leaks with a cleanup function */
        struct pico_websocket_client *client = PICO_ZALLOC(sizeof(struct pico_websocket_client));

        if (!client)
        {
                dbg("Could not allocate websocket client.\n");
                return NULL;
        }

        return client;
}

static int determine_payload_length(struct pico_websocket_client* client, struct pico_websocket_header *hdr)
{
        int ret;
        uint8_t payload_length = hdr->payload_length;

        switch(payload_length)
        {
        case WS_16_BIT_PAYLOAD_LENGTH_INDICATOR:
                ret = backend_read(client->fd, &payload_length, PAYLOAD_LENGTH_SIZE_16_BIT_IN_BYTES);
                break;
        case WS_64_BIT_PAYLOAD_LENGTH_INDICATOR:
                ret = backend_read(client->fd, &payload_length, PAYLOAD_LENGTH_SIZE_64_BIT_IN_BYTES);
                break;
        default:
                /* No extra reading required, size is already known */
                break;
        }
        if (ret < 0)
        {
                return -1;
        }
        return payload_length;
}


/**
 * This function will open a new websocket client.
 * To contact the websocket server, you have to call pico_websocket_client_initiate_connection().
 * Format of the uri must be the following: "ws:" "//" host [ ":" port ] path [ "?" query ] .
 * Events that can occur: EV_WS_ERR and EV_WS_BODY.
 *
 * @param uri This is uri of the server you want to contact.
 */

WSocket ws_connect(char *uri, char *proto, char *ext)
{
        int ret;
        WSocket client;
        struct addrinfo *res, hint;
        int err = 0;

        memset(&hint, 0, sizeof(struct addrinfo));
        hint.ai_family = AF_INET;

        client = build_pico_websocket_client();

        if (client == NULL)
        {
                dbg("Client could not be allocated!\n");
                pico_err = PICO_ERR_ENOMEM;
                return NULL;
        }

        ret = pico_process_URI(uri, &client->uriKey);
        if (ret < 0)
        {
                pico_websocket_client_cleanup(client);
                dbg("URI could not be processed.\n");
                pico_err = PICO_ERR_EINVAL;
                return NULL;
        }


       if (pico_getaddrinfo(client->uriKey.host, NULL, NULL, &res) < 0) {
                dbg("Cannot resolve URI.\n");
                return NULL;
       }
       memcpy(&client->addr, res->ai_addr, sizeof(struct sockaddr_in));
       freeaddrinfo(res);

       client->addr.sin_port = htons(client->uriKey.port);

       client->fd = pico_newsocket(AF_INET, SOCK_STREAM, 0);
       if (client->fd < 0) {
            err = pico_err;
            pico_websocket_client_cleanup(client);
            pico_err = err;
            return NULL;
       }

       if (proto) {
           client->protocol = PICO_ZALLOC(strlen(proto));
           if (!client->protocol) {
                dbg("Warning: cannot allocate buffer for protocol definition\n");
           } else {
               strcpy(client->protocol, proto);
           }
       }
       if (ext) {
           client->extensions = PICO_ZALLOC(strlen(ext));
           if (!client->extensions) {
                dbg("Warning: cannot allocate buffer for extensions definition\n");
           } else {
               strcpy(client->extensions, ext);
           }
       }


       /* TODO: if one of the next steps fail, put a timer to retry the connection
        * instead of destroying everything. That's why we still have a WS_CONNECTING state.
        */
       if (0 != pico_connect(client->fd, (struct sockaddr *)&client->addr, sizeof(struct sockaddr_in))) {
            err = pico_err;
            pico_websocket_client_cleanup(client);
            pico_err = err;
            return NULL;
       }

       if (pico_websocket_client_send_upgrade_header(client) <= 0) {
            err = pico_err;
            pico_websocket_client_cleanup(client);
            pico_err = err;
            return NULL;
       }

       if (ws_parse_upgrade_header(client) < 0)
       {
               err = pico_err;
               pico_websocket_client_cleanup(client);
               pico_err = err;
               return NULL;
       }

       client->state = WS_CONNECTED;
#ifdef SSL_WEBSOCKET
       client->ssl_ctx = wolfSSL_CTX_new(wolfSSLv3_client_method());
       if (!client->ssl_ctx) {
            printf("Failed to create TLS context!\n");
            pico_websocket_client_cleanup(client);
            return NULL;
       }
       if ((wolfSSL_CTX_use_certificate_buffer(client->ssl_ctx, ssl_cert_pem, ssl_cert_pem_len, SSL_FILETYPE_PEM) == 0) ||
               wolfSSL_CTX_use_PrivateKey_buffer(client->ssl_ctx, ssl_cert_pem, ssl_cert_pem_len, SSL_FILETYPE_PEM) == 0) {
            printf("Failed to load TLS certificate or private key!\n");
            pico_websocket_client_cleanup(client);
            return NULL;
       }
       client->ssl = wolfSSL_new(client->ssl_ctx);
       if (client->ssl == NULL) {
            printf("Failed to enable SSL!\n");
            pico_websocket_client_cleanup(client);
            return NULL;
       }
       wolfSSL_set_fd(client->ssl, client->fd);
#endif
       return client;
}

/**
 * This function will close the websocket client.
 * Close messages will be sent before cleaning up the client.
 *
 * @return Will return < 0 if an error occured. Will return 0 if succesfull.
 */
int ws_close(WSocket ws)
{
    return fail_websocket_connection(ws);
}

/**
 * This function will read data received from the websocket server.
 * This function can be called after receiving a WS_EV_BODY in your callback provided in pico_websocket_client_open().
 * If size is smaller than the data received, the data will be truncated.
 *
 * @param data This is the buffer that will be used to hold your data.
 * @param size This is the size of the data you want to read.
 * @return Will return < 0 if an error occured. Will return number of bytes read if succesfull.
 */
int ws_read(WSocket ws, void *data, int size)
{
        int ret;
        int payload_length;
        struct pico_websocket_header hdr;
        int len = 0;
        if (!ws)
        {
                dbg("Wrong Web Socket in ws_read!\n");
                pico_err = PICO_ERR_EINVAL;
                return -1;
        }

        while (1 < 2) {
            while (WEBSOCKET_COMMON_HEADER_SIZE != backend_read(ws->fd, &hdr, WEBSOCKET_COMMON_HEADER_SIZE)) { 
                /* Busy loop: actually blocking on backend_read */
            }
    
            payload_length = determine_payload_length(ws, &hdr);
            if (payload_length < 0)
            {
                    dbg("Could not determine payload length.\n");
                    return -1;
            }
    
            if (size > payload_length)
                size = payload_length;
    
            if ((hdr.opcode != WS_TEXT_FRAME) && (hdr.opcode != WS_BINARY_FRAME) && 
                    (hdr.opcode != WS_CONTINUATION_FRAME) && (hdr.opcode != WS_PING)) {
                if (handle_ctrl_packet(ws, &hdr, payload_length) < 0) /* Received close */ {
                    return -1;
                }
                continue;
            }
    
            /* TODO: Handle continuation correctly. */ 

            while (len < size) {
                ret = backend_read(ws->fd, data + len, size - len);
                if (ret < 0)
                    return -1;
                len += ret;
            }
    
            while (payload_length > size) {
                char c;
                /* Msg had to be truncated due to short usr buffer size.
                 * read out remainings from the TCP buffer. 
                 */
                if (backend_read(ws->fd, &c, 1) < 0)
                    break;
                size++;
            }

            if (hdr.opcode == WS_PING) {
                pico_websocket_client_send_pong(ws, &hdr, data, len);
            }

            return len;
        }
}


/**
 * This function will sent data to the websocket server.
 * If size is greater than 1024 bytes, the message will be fragmented.
 *
 * @param connID This is the connection ID of the websocket client.
 * @param data This is the data you want to send.
 * @param size This is the size of the data you want to send.
 * @return Will return < 0 if an error occured. Will return number of bytes sent if succesfull.
 */

int ws_write_rsv(WSocket ws, void *data, int size, uint8_t *rsv)
{
    struct pico_websocket_header hdr;
    uint32_t masking_key = pico_rand();
    if (!ws)
    {
        dbg("Wrong connID provided to writeData.\n");
        return -1;
    }

    if (size > 0xFFFF)
    {
        pico_err = PICO_ERR_EINVAL;
        return -1;
    }

    if (ws->state != WS_CONNECTED)
    {
            /* We are either not fully initialized, or we are closing our connection.
             * do not send out packets!
             */
            return -1;
    }

    memset(&hdr, 0, sizeof(struct pico_websocket_header));

    if (rsv) {
        hdr.RSV1 = rsv[0];
        hdr.RSV2 = rsv[1];
        hdr.RSV3 = rsv[2];
    }
    hdr.opcode = WS_TEXT_FRAME;
    hdr.mask = WS_MASK_ENABLE;
    hdr.fin = WS_FIN_ENABLE;
    hdr.payload_length = size;
    if (size > WS_7_BIT_PAYLOAD_LENGTH_LIMIT) {
        hdr.payload_length = WS_16_BIT_PAYLOAD_LENGTH_INDICATOR;
    }
    if ( backend_write(ws->fd, &hdr, sizeof(hdr)) < 0)
        return -1;
    if (hdr.payload_length == WS_16_BIT_PAYLOAD_LENGTH_INDICATOR) {
        uint16_t p_size = htons(size);
        if (backend_write(ws->fd, &p_size, sizeof(uint16_t)) < 0)
            return -1;
    }
    if (backend_write(ws->fd, &masking_key, sizeof(uint32_t)) < 0)
        return -1;

    pico_websocket_mask_data(masking_key, data, size);
    return backend_write(ws->fd, data, size);
}

int ws_write(WSocket ws, void *data, int size)
{
    return ws_write_rsv(ws, data, size, NULL);
}

