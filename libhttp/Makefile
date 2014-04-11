PICOTCP?=../../picotcp
CC:=$(CROSS_COMPILE)gcc
LD:=$(CROSS_COMPILE)ld
AR:=$(CROSS_COMPILE)ar
RANLIB:=$(CROSS_COMPILE)ranlib
SIZE:=$(CROSS_COMPILE)size
STRIP_BIN:=$(CROSS_COMPILE)strip
TEST_LDFLAGS=-pthread  $(PREFIX)/modules/*.o $(PREFIX)/lib/*.o -lvdeplug -lpcap
OPTIONS?=

CFLAGS=-I$(PICOTCP)/build/include
CFLAGS += -Wall
CFLAGS += -mcpu=cortex-m3 -mthumb -mlittle-endian -mthumb-interwork
CFLAGS += -ffreestanding -nostartfiles
CFLAGS += -mfloat-abi=soft
CFLAGS += -DCORE_M3 -DLPC18XX
CFLAGS += -DDEBUG_HARDFAULT -ggdb -O0
CFLAGS += -DMEM_MEASURE

PWD=`pwd`


all: libhttp.a

picotcp/lib/libpicotcp.a:
	@[ -d $(PICOTCP) ] || ( echo && echo && echo "PicoTCP path not found. Please run 'make PICOTCP=/path/to/picotcp/src'" && echo && exit 1)
	make -C $(PICOTCP) lib PREFIX=$(PWD)/picotcp $(FEATURES) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) 


#libhttp.a: picotcp/lib/libpicotcp.a
libhttp.a: 
	$(CC) -c -o pico_http_server.o pico_http_server.c $(CFLAGS)
	$(CC) -c -o pico_http_client.o pico_http_client.c $(CFLAGS)
	$(CC) -c -o pico_http_util.o   pico_http_util.c $(CFLAGS)
	$(AR) cru libhttp.a *.o 
	$(RANLIB) libhttp.a


clean:
	rm -rf picotcp
	rm -f *.o *.a
