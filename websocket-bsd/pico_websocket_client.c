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
#endif


#define WS_PROTO_TOK                           "ws://"
#define WS_PROTO_LEN                           5u

#define WSS_PROTO_TOK                          "wss://"
#define WSS_PROTO_LEN                          6u


#define HTTP_HEADER_LINE_SIZE                  128u
#define HTTP_RESPONSE_CODE_INDEX               9u
#define WEBSOCKET_COMMON_HEADER_SIZE           (sizeof(struct pico_websocket_header))

#define PAYLOAD_LENGTH_SIZE_16_BIT_IN_BYTES    2u
#define PAYLOAD_LENGTH_SIZE_64_BIT_IN_BYTES    8u

#define WS_MASKING_KEY_SIZE_IN_BYTES           4u

#define WS_BUFFER_SIZE                         5*1024u /* Provide 5K for fragmented messages (recv or sending) */
#define WS_FRAME_SIZE                          1024u   /* We define one frame as 1K payload */

#define WS_CONTROL_FRAME_MAX_SIZE              125u

#define WS_CLOSING_WAIT_TIME_IN_MS             2000u

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
static int pico_websocket_client_cleanup(struct pico_websocket_client* client);

#ifdef SSL_WEBSOCKET
extern unsigned char ca_pem[];
extern unsigned int ca_pem_len;

static int backend_read(WSocket s, void *data, int len)
{
    return wolfSSL_read(s->ssl, data, len);
}

static int backend_write(WSocket s, void *data, int len)
{
    return wolfSSL_write(s->ssl, data, len);
}

#else
static int backend_read(WSocket s, void *data, int len)
{
    return pico_read(s->fd, data, len);
}

static int backend_write(WSocket s, void *data, int len)
{
    return pico_write(s->fd, data, len);
}
#endif

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

#ifdef SSL_WEBSOCKET
        if(memcmp(uri, WSS_PROTO_TOK, WSS_PROTO_LEN) == 0) /* could be optimized */
        { /* protocol identified, it is wss */
                urikey->protoHttp = 1;
                lastIndex = WSS_PROTO_LEN;
        }
#endif


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

/* If close frame has a payload, the first two bytes in the payload are an unisgned int in network order. This is why we add WS_STATUS_CODE_SIZE_IN_BYTES to the reason_size when checking the size. */
static int send_close_frame(struct pico_websocket_client* client, uint16_t status_code, void* reason, uint8_t reason_size)
{
        int ret;
        uint32_t masking_key;
        struct pico_websocket_header* hdr;

        if (reason_size + WS_STATUS_CODE_SIZE_IN_BYTES > WS_CONTROL_FRAME_MAX_SIZE)
        {
                dbg("Size of reason is too big to fit in close frame.\n");
                return -1;
        }

        hdr = PICO_ZALLOC(sizeof(struct pico_websocket_header));
        hdr->opcode = WS_CONN_CLOSE;
        hdr->fin = WS_FIN_ENABLE;
        hdr->RSV1 = 0;
        hdr->RSV2 = 0;
        hdr->RSV3 = 0;
        hdr->mask = WS_MASK_ENABLE;
        masking_key = pico_rand();

        if (reason && reason_size)
        {
                hdr->payload_length = reason_size + WS_STATUS_CODE_SIZE_IN_BYTES;
        }

        ret = backend_write(client, hdr, sizeof(struct pico_websocket_header));

        if (ret < 0)
        {
                dbg("Failed to write header for close frame.\n");
                free(hdr);
                return -1;
        }

        ret = backend_write(client, &masking_key, WS_MASKING_KEY_SIZE_IN_BYTES);

        if (ret < 0)
        {
                dbg("Failed to write header for close frame.\n");
                free(hdr);
                return -1;
        }

        if (reason && reason_size)
        {
                /* When no status code is provided, the server will default to 1005, so not our responsibility */
                if (status_code)
                {
                        status_code = short_be(status_code);
                        status_code = pico_websocket_mask_data(masking_key, (uint8_t *)&status_code, WS_STATUS_CODE_SIZE_IN_BYTES );
                        ret = backend_write(client, &status_code, WS_STATUS_CODE_SIZE_IN_BYTES);
                }
                pico_websocket_mask_data(masking_key, reason, reason_size);
                ret = backend_write(client, reason, reason_size);
        }

        free(hdr);
        return ret;
}


/* TODO: normally when you sent a close frame you have to wait a reasonable amount of time for the server to send you a close frame as well. For now, we close the connection immediately.*/
static int fail_websocket_connection(struct pico_websocket_client* client)
{
        switch (client->state)
        {
        case WS_CONNECTED:
                /* I recv a close frame, I should sent one back. Now I am closed*/
                send_close_frame(client, 0, NULL, 0);
                client->state = WS_CLOSED;
                pico_websocket_client_cleanup(client);
                /* pico_timer_add(WS_CLOSING_WAIT_TIME_IN_MS, cleanup_websocket_client, (void*)client); */
                break;
        case WS_CLOSING:
                /* I sent a close frame and am waiting for the servers response */
                /* pico_timer_add(WS_CLOSING_WAIT_TIME_IN_MS, cleanup_websocket_client, (void*)client); */
                pico_websocket_client_cleanup(client);
                break;
        case WS_CLOSED:
                break;
        default:
                /* pico_timer_add(WS_CLOSING_WAIT_TIME_IN_MS, cleanup_websocket_client, (void*)client); */
                pico_websocket_client_cleanup(client);
                break;
        }

        return 0;
}



static int pico_websocket_client_cleanup(struct pico_websocket_client* client)
{
        dbg("Closing the websocket client...\n");

        if (client->fd >= 0) {
            pico_close(client->fd);
            client->fd = -1;
        }
#ifdef SSL_WEBSOCKET
        if (client->ssl_ctx)
                wolfSSL_CTX_free(client->ssl_ctx);

        if (client->ssl)
        {
                wolfSSL_shutdown(client->ssl);
                wolfSSL_free(client->ssl);
        }
#endif

        if (client->extensions)
        {
                PICO_FREE(client->extensions);
        }

        if (client->extension_arguments)
        {
                PICO_FREE(client->extension_arguments);
        }

        if (client->uriKey.host)
                PICO_FREE(client->uriKey.host);
        if (client->uriKey.resource)
                PICO_FREE(client->uriKey.resource);

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
    return backend_read(ws, c, 1);
}

/* Note: "\r\n" is not considered a line, so the last line of a http message will give back strlen(line) == 0 */
static void ws_read_http_header_line(struct pico_websocket_client* client, char *line, int line_len)
{
        char c;
        uint32_t index = 0;

        while(read_char_from_client_socket(client, &c) > 0 && c != '\r' && index < (line_len - 1))
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
        char* header;
        int header_size = 256;

        header = PICO_ZALLOC(header_size);
        strcpy(header, "GET ");
        strcat(header, client->uriKey.resource);
        strcat(header, " HTTP/1.1\r\n");
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

        ret = backend_write(client, header, strlen(header));

        if(ret < 0)
        {
                dbg("Failed to send upgrade header.\n");
                ret = -1;
        }

        free(header);

        return ret;
}


static int parse_server_http_response_line(char* line, char* colon_ptr)
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

        ws_read_http_header_line(client, line, HTTP_HEADER_LINE_SIZE);

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
                free(line);
                return -1;
        default:
                free(line);
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
                ws_read_http_header_line(client, line, HTTP_HEADER_LINE_SIZE);
                colon_ptr = strstr(line, ":");
                if (colon_ptr != NULL)
                {
                        ret = parse_server_http_response_line(line, colon_ptr);
                        if (ret < 0)
                        {
                                /* TODO: cancel everything */
                        }
                }
        } while ( strlen(line) != 0 );

        free(line);
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
    backend_write(ws, hdr, sizeof(struct pico_websocket_header));
    backend_write(ws, &masking_key, sizeof(uint32_t));
    if (size > 0) {
        pico_websocket_mask_data(masking_key, data, size);
        backend_write(ws, data, size);
        pico_websocket_mask_data(masking_key, data, size); /* Unmask data in place, that's going back to user! */
    }
}

static WSocket build_pico_websocket_client(void)
{
        struct pico_websocket_client *client = PICO_ZALLOC(sizeof(struct pico_websocket_client));

        if (!client)
        {
                dbg("Could not allocate websocket client.\n");
                return NULL;
        }

        /* Set fd to an invalid value, no socket opened yet */
        client->fd = -1;

        return client;
}

static int determine_payload_length(struct pico_websocket_client* client, struct pico_websocket_header *hdr)
{
        int ret = -1;
        uint8_t payload_length = hdr->payload_length;

        switch(payload_length)
        {
        case WS_16_BIT_PAYLOAD_LENGTH_INDICATOR:
                ret = backend_read(client, &payload_length, PAYLOAD_LENGTH_SIZE_16_BIT_IN_BYTES);
                break;
        case WS_64_BIT_PAYLOAD_LENGTH_INDICATOR:
                ret = backend_read(client, &payload_length, PAYLOAD_LENGTH_SIZE_64_BIT_IN_BYTES);
                break;
        default:
                /* No extra reading required, size is already known */
                ret = 1; /* Just so ret is not zero */
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
 * Format of the uri must be the following: "ws:" "//" host [ ":" port ] path [ "?" query ] or
 * "wss:" "//" host [ ":" port ] path [ "?" query ] for secure connections.
 *
 * @param uri This is uri of the server you want to contact.
 * @param proto This is a comma-separated list that will be used in the upgrade header to negotiate
 * protocols. If no protocols are given, this will default to chat and superchat.
 * @param ext This is a comma-separated list that will be used in the upgrade header to negotiate
 * extensions on the websocket protocol. If no extensions are given, this will default to NULL
 * @return Will return a WSocket client or NULL if something went wrong.
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
                pico_websocket_client_cleanup(client);
                dbg("Cannot resolve URI.\n");
                return NULL;
       }
       memcpy(&client->addr, res->ai_addr, sizeof(struct sockaddr_in));
       pico_freeaddrinfo(res);

       client->addr.sin_port = short_be(client->uriKey.port);

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

#ifdef SSL_WEBSOCKET
       client->ssl_ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
       if (!client->ssl_ctx) {
               dbg("Failed to create TLS context!\n");
               pico_websocket_client_cleanup(client);
               return NULL;
       }
       
       if (wolfSSL_CTX_load_verify_buffer(client->ssl_ctx, ca_pem, ca_pem_len, SSL_FILETYPE_PEM) != SSL_SUCCESS)
       {
               dbg("failed to load verify buffer!\n");
               pico_websocket_client_cleanup(client);
               return NULL;
       }
       
       wolfSSL_CTX_set_verify(client->ssl_ctx, SSL_VERIFY_PEER, NULL);

       client->ssl = wolfSSL_new(client->ssl_ctx);
       if (client->ssl == NULL) {
               dbg("Failed to enable SSL!\n");
               pico_websocket_client_cleanup(client);
               return NULL;
       }
       wolfSSL_set_fd(client->ssl, client->fd);

       /* The line below is needed for ECC keys using the secp256r1 */
       wolfSSL_UseSupportedCurve(client->ssl, WOLFSSL_ECC_SECP256R1);

#endif

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
       return client;
}

/**
 * This function will close the websocket client.
 * Close messages will be sent before cleaning up the client.
 * @param ws This is the websocket client returned by ws_connect()
 *
 * @return Will return < 0 if an error occured. Will return 0 if succesfull.
 */
int ws_close(WSocket ws)
{
    return fail_websocket_connection(ws);
}

/**
 * This function will read data received from the websocket server.
 * If size is smaller than the data received, the data will be truncated.
 *
 * @param ws This is the websocket client returned by ws_connect()
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
            if (WEBSOCKET_COMMON_HEADER_SIZE != backend_read(ws, &hdr, WEBSOCKET_COMMON_HEADER_SIZE)) {
                    /* Websocket header could not be read */
                    return -1;
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
                ret = backend_read(ws, data + len, size - len);
                if (ret < 0)
                    return -1;
                len += ret;
            }
    
            while (payload_length > size) {
                char c;
                /* Msg had to be truncated due to short usr buffer size.
                 * read out remainings from the TCP buffer. 
                 */
                if (backend_read(ws, &c, 1) < 0)
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
 * What kind of data is being sent, can be selected with the opcode parameter.
 * The difference between this function and ws_write() is that you can provide values for the rsv bits
 * used in the header sent to the server.
 * @param ws This is the websocket returned by ws_connect()
 * @param data This is the data you want to send.
 * @param size This is the size of the data you want to send.
 * @param rsv These are the values of the rsv bits. This is an 3-element array laid out like this:
 * rsv = [ rsv1, rsv2, rsv3 ].
 * The only valid values for rsv bits are RSV_ENABLE (1) and RSV_DISABLE (0).
 * @param opcode This is the opcode you want to send with the websocket header. This can be one of the opcodes found in pico_websocket_util.h . Normally you want WS_TEXT_FRAME or WS_BINARY_FRAME.
 * @return Will return < 0 if an error occured. Will return number of bytes sent if succesfull.
 */
int ws_write_rsv(WSocket ws, void *data, int size, uint8_t *rsv, uint8_t opcode)
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
    hdr.opcode = opcode;
    hdr.mask = WS_MASK_ENABLE;
    hdr.fin = WS_FIN_ENABLE;
    hdr.payload_length = size;
    if (size > WS_7_BIT_PAYLOAD_LENGTH_LIMIT) {
        hdr.payload_length = WS_16_BIT_PAYLOAD_LENGTH_INDICATOR;
    }
    if ( backend_write(ws, &hdr, sizeof(hdr)) < 0)
        return -1;
    if (hdr.payload_length == WS_16_BIT_PAYLOAD_LENGTH_INDICATOR) {
        uint16_t p_size = short_be(size);
        if (backend_write(ws, &p_size, sizeof(uint16_t)) < 0)
            return -1;
    }
    if (backend_write(ws, &masking_key, sizeof(uint32_t)) < 0)
        return -1;

    pico_websocket_mask_data(masking_key, data, size);
    return backend_write(ws, data, size);
}


/**
 * This function will sent text data to the websocket server.
 * The difference between this function and ws_write_rsv() is that you can't provide values for the rsv bits used in the header sent to the server.
 * The RSV bits in the header will be zero.
 * @param ws This is the websocket returned by ws_connect()
 * @param data This is the data you want to send.
 * @param size This is the size of the data you want to send.
 * @return Will return < 0 if an error occured. Will return number of bytes sent if succesfull.
 */
int ws_write(WSocket ws, void *data, int size)
{
    return ws_write_rsv(ws, data, size, NULL, WS_TEXT_FRAME);
}

/**
 * This function will sent binary data to the websocket server.
 * The difference between this function and ws_write_rsv() is that you can't provide values for the rsv bits used in the header sent to the server.
 * The RSV bits in the header will be zero.
 * @param ws This is the websocket returned by ws_connect()
 * @param data This is the data you want to send.
 * @param size This is the size of the data you want to send.
 * @return Will return < 0 if an error occured. Will return number of bytes sent if succesfull.
 */
int ws_write_data(WSocket ws, void *data, int size)
{
    return ws_write_rsv(ws, data, size, NULL, WS_BINARY_FRAME);
}

