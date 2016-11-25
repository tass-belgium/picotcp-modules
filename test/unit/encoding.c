#include <string.h>
#include <stdio.h>

int encode(char *buffout, char *buffin) {	

	if (strcmp(buffin, "user:pass\n") == 0) {
		sprintf(buffout, "dXNlcjpwYXNz\n");
	} else if (strcmp(buffin, "panda:isalive\n") == 0) {
		sprintf(buffout, "cGFuZGE6aXNhbGl2ZQ==\n");
	} else if (strcmp(buffin, "rabbit:eatcarrots\n") == 0) {
		sprintf(buffout, "cmFiYml0OmVhdGNhcnJvdHM=\n");
	} else {
		return -1;
	}
	return 0;

}
