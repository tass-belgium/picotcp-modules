/* 
 * This module glues the HTTP and TLS libraries together, resulting in HTTPS.
 * Essentially, it defines and/or implements all library-specific code. Start here
 * should we need any more SSL/TLS implementations spliced with picoTCP ;)
 *
 * Author: Alexander Zuliani <alexander.zuliani@tass.be>
 */

#ifndef PICO_HTTPS_GLUE_H_
#define PICO_HTTPS_GLUE_H_

// Everyone needs pico!
#include "pico_https_server.h"
#include "pico_stack.h"
#include "pico_tcp.h"
#include "pico_tree.h"

// Check configuration sanity. Warn where appropriate
#if !(defined(LIBHTTPS_USE_POLARSSL) || defined(LIBHTTPS_USE_WOLFSSL))
    #error "Please define either LIBHTTPS_USE_POLARSSL or LIBHTTPS_USE_WOLFSSL"
#elif (defined(LIBHTTPS_USE_POLARSSL) && defined(LIBHTTPS_USE_WOLFSSL))
    #warning "Both LIBHTTPS_USE_POLARSSL and LIBHTTPS_USE_WOLFSSL are defined. Defaulting to PolarSSL!"
    #undef LIBHTTPS_USE_WOLFSSL
#endif

#if defined(LIBHTTPS_USE_POLARSSL)
    // Includes for PolarSSL
    #include "polarssl/entropy.h"
    #include "polarssl/ctr_drbg.h"
    #include "polarssl/x509.h"
    #include "polarssl/ssl.h"
    #include "polarssl/error.h"
    #include "polarssl/debug.h"

    // Function bindings for PolarSSL
    #define SSL_CONTEXT     ssl_context         // Generally treated as void*. Use any encapsulating struct that suits your needs

    #define SSL_WRITE       ssl_write           // We expect a function with signature SSL_{READ/WRITE}(SSL_CONTEXT* , unsigned char* buf, int len)
    #define SSL_READ        ssl_read            // Both return num bytes successfully read/written

    #define SSL_HANDSHAKE   ssl_handshake       // We expect int SSL_HANDSHAKE(SSL_CONTEXT*), returning 0 for "Handshake complete".
    
    #define SSL_FREE        ssl_free            // We expect void SSL_FREE(SSL_CONTEXT*), destroying all context and references therein
    #define SSL_SHUTDOWN    ssl_close_notify    // This isn't even required by the standard. Send "Alert: Close" (void SSL_SHUTDOWN(SSL_CONTEXT*))

    // Callback signatures specific to PolarSSL (implement these in the implementation-appropriate pico_https_glue_*.c file)
    int pico_polar_recv(void * pico_sock, unsigned char * buf, size_t sz);         
    int pico_polar_send(void * pico_sock, const unsigned char * buf, size_t sz);

#elif defined(LIBHTTPS_USE_WOLFSSL)
    // Includes for wolfSSL
    #include "wolfssl/ssl.h"

    // Function bindings for wolfSSL
    #define SSL_CONTEXT     WOLFSSL
    #define SSL_WRITE       wolfSSL_write
    #define SSL_READ        wolfSSL_read
    int SSL_HANDSHAKE(WOLFSSL* ssl); // We need to tweak retvals, can't directly map
    #define SSL_FREE        wolfSSL_free
    #define SSL_SHUTDOWN    wolfSSL_shutdown

    // Callback signatures specific to wolfSSL
    int pico_wolfssl_recv(WOLFSSL *ssl, char *buf, int sz, void *ctx);
    int pico_wolfssl_send(WOLFSSL *ssl, char *buf, int sz, void *ctx);

#endif

/* 
 * Generic signatures (implement these in ALL pico_https_glue_*.c files)
 */

// General setup (initialize entropy, load certs, called ONCE)
int pico_https_ssl_init(const unsigned char* certificate_buffer,
                         const unsigned int   certificate_buffer_size,
                         const unsigned char* privkey_buffer,
                         const unsigned int   privkey_buffer_size);

// Connection setup (create context, bind socket, set callbacks, once PER CONNECTION)
SSL_CONTEXT* pico_https_ssl_accept(struct pico_socket* sock);  

#endif
