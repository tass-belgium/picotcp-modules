/*
 * All CyaSSL-specific handling of TLS connections is done here.
 * Many of the specific code is dealt with in the header file (DEFINES)
 *
 * Author: Alexander Zuliani <alexander.zuliani@tass.be> 
*/

#include "pico_https_glue.h" // Generic config

#ifdef LIBHTTPS_USE_CYASSL // This all makes no sense otherwise

// Globals we're going to need
CYASSL_CTX* SSL_Context;

// GLOBAL init
void pico_https_ssl_init(const unsigned char* certificate_buffer,
                         const unsigned int   certificate_buffer_size,
                         const unsigned char* privkey_buffer,
                         const unsigned int   privkey_buffer_size){
    CyaSSL_Init();

    SSL_Context = CyaSSL_CTX_new( CyaTLSv1_server_method() );
    CyaSSL_SetIORecv(SSL_Context, pico_cyassl_recv);
    CyaSSL_SetIOSend(SSL_Context, pico_cyassl_send);
    CyaSSL_CTX_use_certificate_buffer( SSL_Context, certificate_buffer, certificate_buffer_size, SSL_FILETYPE_PEM );
    CyaSSL_CTX_use_PrivateKey_buffer( SSL_Context, privkey_buffer, privkey_buffer_size, SSL_FILETYPE_PEM );
}

/* PER-CONNECTION initialisation */
CYASSL* pico_https_ssl_accept(struct pico_socket* sck){
    // Register sockets and metadata as Context for the glue
    CYASSL* ret = CyaSSL_new(SSL_Context);
    CyaSSL_set_using_nonblock(ret, 1);
    CyaSSL_SetIOReadCtx(ret, sck);
    CyaSSL_SetIOWriteCtx(ret, sck);
    return ret;
}

/* IO Callbacks */
int pico_cyassl_send(CYASSL* ssl, char *buf, int sz, void *ctx)
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

int pico_cyassl_recv(CYASSL *ssl, char *buf, int sz, void *ctx)
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

int SSL_HANDSHAKE(CYASSL* ssl){
    int ret = CyaSSL_accept(ssl);
    if (ret == SSL_SUCCESS) // Fuck you CyaSSL. Who defines OK != 0?
        return 0;
    return -1;
}

#endif // ifdef LIBHTTPS_USE_CYASSL
