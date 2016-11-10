#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/* return the configuration for the IP lookup*/
static struct addrinfo lookup_configuration( void ){
	struct addrinfo hints;
	int* flags = &(hints.ai_flags);


	/* configure the hints struct */
	hints.ai_family = AF_INET6; /* allow only IPv6 address, IPv4 will be mapped to IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* only use TCP sockets */
	hints.ai_protocol = 0; /*allow any protocol //TODO check for restrictions */
	
	/* clear the flags before setting them*/
	*flags = 0;

	/* set the appropriate flags*/
#if (PICO_MQTT_DNS_LOOKUP == 1)
	*flags |= AI_NUMERICHOST;
#endif /*(PICO_MQTT_DNS_LOOKUP = 1)*/
	/* AI_PASSIVE unsed, intend to use address for a connect*/
	/* AI_NUMERICSERV unsed, //TODO check for this restriction*/
#if (PICO_MQTT_HOSTNAME_LOOKUP ==1)
	*flags |= IA_CANONNAME;
#endif /*(PICO_MQTT_HOSTNAME_LOOKUP ==1)*/
	/*AI_ADDRCONFIG unsed, intedded to use IPv6 of mapped IPv4 addresses*/
	*flags |= AI_V4MAPPED; /* map IPv4 addresses to IPv6 addresses*/
	*flags |= AI_ALL; /* return both IPv4 and IPv6 addresses*/

	/* set the unused variables to 0 */
	hints.ai_addrlen = 0;
	hints.ai_addr = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;

	return hints;
}

int lookup_host (const char *host)
{
  struct addrinfo hints, *res;
  int errcode;
  char addrstr[100];
  void *ptr;

	hints = lookup_configuration();

  errcode = getaddrinfo ("localhost", "1883", &hints, &res);
  if (errcode != 0)
    {
      printf("error\n");
      perror ("getaddrinfo error");
      return -1;
    }

  printf ("Host: %s\n", host);
  while (res)
    {
      inet_ntop (res->ai_family, &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr, addrstr, 100);
      printf ("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4,
              addrstr, res->ai_canonname);
      res = res->ai_next;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  return lookup_host (argv[1]);
}
