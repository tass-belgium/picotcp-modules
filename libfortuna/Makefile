PICOTCP?=../../picotcp
CYASSL?=../../cyassl
CC:=$(CROSS_COMPILE)gcc
LD:=$(CROSS_COMPILE)ld
AR:=$(CROSS_COMPILE)ar
RANLIB:=$(CROSS_COMPILE)ranlib
SIZE:=$(CROSS_COMPILE)size
STRIP_BIN:=$(CROSS_COMPILE)strip
TEST_LDFLAGS=-pthread  $(PREFIX)/modules/*.o $(PREFIX)/lib/*.o -lvdeplug -lpcap
OPTIONS?=

CFLAGS=-I$(PICOTCP)/include -I$(CYASSL)
CFLAGS += -Wall
PWD=`pwd`


all: libfortuna.a

picotcp/lib/libpicotcp.a:
	@[ -d $(PICOTCP) ] || ( echo && echo && echo "PicoTCP path not found. Please run 'make PICOTCP=/path/to/picotcp/src'" && echo && exit 1)
	make -C $(PICOTCP) lib PREFIX=$(PWD)/picotcp $(FEATURES) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) 


libfortuna.a:
	$(CC) -c -o pico_rand.o pico_rand.c $(CFLAGS)
	$(CC) -c -o pico_rand_types.o pico_rand_types.c $(CFLAGS)
	$(AR) cru libfortuna.a *.o 
	$(RANLIB) libfortuna.a


clean:
	rm -rf picotcp
	rm -f *.o *.a
