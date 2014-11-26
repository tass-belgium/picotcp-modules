-include ../../../config.mk

PREFIX?=$(PWD)/build
CROSS_COMPILE?=arm-none-eabi-
CC?=gcc
ARCH?=stm32
OBJS:=pico_https_server.o pico_https_util.o pico_https_glue.o
CFLAGS=-Iconfig $(EXTRA_CFLAGS) $(PLATFORM_CFLAGS) -I $(PREFIX)/include -DCYASSL_PICOTCP -DCYASSL_USER_IO -DNO_WRITEV
CFLAGS+=-DPOLARSSL_DEBUG_MSG -DPOLARSSL_DEBUG_C

all: $(PREFIX)/lib/libhttps.a

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PREFIX)/lib/libhttps.a: $(OBJS)
	cp *.h $(PREFIX)/include
	@$(CROSS_COMPILE)ar cru $@ $(OBJS)
	@$(CROSS_COMPILE)ranlib $@

clean:
	rm -f $(OBJS)
