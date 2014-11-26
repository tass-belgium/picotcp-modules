#include "pico_https_glue.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"
#include "polarssl/certs.h"
#include "polarssl/x509.h"
#include "polarssl/ssl.h"
#include "polarssl/error.h"

int pico_EmbedSend(void * pico_sock, const unsigned char * buf, size_t sz)
{
    struct pico_socket *sock = (struct pico_socket *) pico_sock;
    int sent;
    int len = sz;

    sent = (int)pico_socket_write(sock, buf, len);
	if (sent != 0)
		return sent;
	else
		return -2;
}

int pico_EmbedReceive(void * pico_sock, unsigned char * buf, size_t sz)
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
