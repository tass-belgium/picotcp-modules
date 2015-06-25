/*
 * All WolfSSL-specific handling of TLS connections is done here.
 * Many of the specific code is dealt with in the header file (DEFINES)
 *
 * Author: Alexander Zuliani <alexander.zuliani@tass.be> 
*/

#include "pico_https_glue.h" // Generic config
#include "wolfssl/ssl.h"

#ifdef LIBHTTPS_USE_WOLFSSL // This all makes no sense otherwise

// Globals we're going to need
WOLFSSL_CTX* SSL_Context;

// GLOBAL init
int pico_https_ssl_init(const unsigned char* certificate_buffer,
                         const unsigned int   certificate_buffer_size,
                         const unsigned char* privkey_buffer,
                         const unsigned int   privkey_buffer_size){
    wolfSSL_Init();

    SSL_Context = wolfSSL_CTX_new( wolfTLSv1_server_method() );
    if (!SSL_Context)
        return -1;

    wolfSSL_SetIORecv(SSL_Context, pico_wolfssl_recv);
    wolfSSL_SetIOSend(SSL_Context, pico_wolfssl_send);
    if (wolfSSL_CTX_use_certificate_buffer( SSL_Context, certificate_buffer, certificate_buffer_size, SSL_FILETYPE_PEM ) != SSL_SUCCESS)
        return -1;
    if (wolfSSL_CTX_use_PrivateKey_buffer( SSL_Context, privkey_buffer, privkey_buffer_size, SSL_FILETYPE_PEM ) != SSL_SUCCESS)
        return -1;

    return 0;
}

/* PER-CONNECTION initialisation */
WOLFSSL* pico_https_ssl_accept(struct pico_socket* sck){
    // Register sockets and metadata as Context for the glue
    WOLFSSL* ret = wolfSSL_new(SSL_Context);
    wolfSSL_set_using_nonblock(ret, 1);
    wolfSSL_SetIOReadCtx(ret, sck);
    wolfSSL_SetIOWriteCtx(ret, sck);
    return ret;
}

/* IO Callbacks */
int pico_wolfssl_send(WOLFSSL* ssl, char *buf, int sz, void *ctx)
{
    struct pico_socket *sock = (struct pico_socket *) ctx;
    int sent;
    int len = sz;

    sent = (int)pico_socket_write(sock, buf, len);
    if (sent != 0)
        return sent;
    else
        return -2;
}

int pico_wolfssl_recv(WOLFSSL *ssl, char *buf, int sz, void *ctx)
{
    struct pico_socket *sock = (struct pico_socket *) ctx;
    int read;
    int len = sz; 

    read = (int)pico_socket_read(sock, buf, len);
    if (read != 0)
        return read;
    else
        return -2; // WANT_READ. Needed for non_blocking reads
}

int SSL_HANDSHAKE(WOLFSSL* ssl){
    int ret = wolfSSL_accept(ssl);
    if (ret == SSL_SUCCESS) /* SSL_SUCCESS apparently != 0 for some reason. */
        return 0;
    return -1;
}

#endif // ifdef LIBHTTPS_USE_WOLFSSL
