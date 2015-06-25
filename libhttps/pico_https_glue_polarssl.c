/*
 * All PolarSSL-specific handling of TLS connections is done here.
 * Many of the specific code is dealt with in the header file (DEFINES)
 *
 * Author: Alexander Zuliani <alexander.zuliani@tass.be> 
*/

#include "pico_https_glue.h" // Generic config

#ifdef LIBHTTPS_USE_POLARSSL // This all makes no sense otherwise

// A bunch of globals we're going to need
entropy_context entropy;
ctr_drbg_context ctr_drbg;
x509_crt srvcert;
pk_context pkey;

// GLOBAL init
int pico_https_ssl_init(const unsigned char* certificate_buffer,
                         const unsigned int   certificate_buffer_size,
                         const unsigned char* privkey_buffer,
                         const unsigned int   privkey_buffer_size){
    x509_crt_init( &srvcert );
    pk_init( &pkey );
    entropy_init( &entropy );

    // This part would benefit from error handling
    x509_crt_parse( &srvcert, certificate_buffer, certificate_buffer_size );
    pk_parse_key(   &pkey,    privkey_buffer,     privkey_buffer_size, NULL, 0 );
    // No clue what the last two arguments (NULL,0) are. Someone should probably check the API

    // This isn't how entropy works. This isn't how any of this works.
    ctr_drbg_init( &ctr_drbg, entropy_func, &entropy, (const unsigned char *) "lalala", 6 );

    // TODO: actually check the return values of all above functions!
    return 0;
}

/* PER-CONNECTION initialisation */
ssl_context* pico_https_ssl_accept(struct pico_socket* sck){

    ssl_context* ret = PICO_ZALLOC(sizeof(ssl_context));
	ssl_init(ret);
    ssl_set_endpoint( ret, SSL_IS_SERVER );
    ssl_set_authmode( ret, SSL_VERIFY_NONE );
    ssl_set_ca_chain( ret, srvcert.next, NULL, NULL );
    ssl_set_own_cert( ret, &srvcert, &pkey ); 
	ssl_set_rng( ret, ctr_drbg_random, &ctr_drbg );
    ssl_set_bio( ret, pico_polar_recv, sck,
                      pico_polar_send, sck );
	return ret;
}
    

int pico_polar_send(void * pico_sock, const unsigned char * buf, size_t sz)
{
    struct pico_socket *sock = (struct pico_socket *) pico_sock;
    int sent;
    int len = sz;

    sent = (int)pico_socket_write(sock, buf, len);
	if (sent != 0)
		return sent;
	else
		return -2; // WANT_WRITE. For non_blocking writes
}

int pico_polar_recv(void * pico_sock, unsigned char * buf, size_t sz)
{
	struct pico_socket *sock = (struct pico_socket *) pico_sock;
    int read;
    int len = sz; 

    read = (int)pico_socket_read(sock, buf, len);
	if (read != 0)
		return read;
	else
		return -2; // WANT_READ. Needed for non_blocking reads
}

#endif // ifdef LIBHTTPS_USE_POLARSSL
