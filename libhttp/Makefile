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
ifeq ($(CROSS_COMPILE),arm-none-eabi-)
CFLAGS += -mcpu=cortex-m3 -mthumb -mlittle-endian -mthumb-interwork
CFLAGS += -ffreestanding -nostartfiles
CFLAGS += -mfloat-abi=soft
CFLAGS += -DCORE_M3 -DLPC18XX
CFLAGS += -DDEBUG_HARDFAULT -ggdb -O0
CFLAGS += -DMEM_MEASURE
endif
ifeq ($(ARCH),faulty)
CFLAGS += -DUNIT_TEST
endif
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

units: libhttp.a
	gcc -o units.app $^ -I./ $(CFLAGS) ../test/unit/modunit_pico_http_client.c -lcheck -lm -pthread -lrt libhttp.a

clean:
	rm -rf picotcp
	rm -f *.o *.a ../test/unit/*.o
