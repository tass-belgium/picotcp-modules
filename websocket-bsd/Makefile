-include ../../../config.mk
-include ../../../tools/kconfig/.config


PREFIX?=$(PWD)/out
CROSS_COMPILE?=arm-none-eabi-
CC?=gcc
OBJS:=pico_websocket_client.o
CFLAGS=-Iconfig $(EXTRA_CFLAGS) $(PLATFORM_CFLAGS) -I $(PREFIX)/include

ifeq ($(UNIT_TEST),1)
  CFLAGS+=-DUNIT_TEST
endif

ifeq ($(SSL_WEBSOCKET),1)
  CFLAGS+=-DSSL_WEBSOCKET
endif

all: $(PREFIX)/lib/libpicowebsocket_bsd.a

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PREFIX)/lib/libpicowebsocket_bsd.a: $(OBJS)
	cp *.h $(PREFIX)/include
	@$(CROSS_COMPILE)ar cru $@ $(OBJS)
	@$(CROSS_COMPILE)ranlib $@

clean:
	rm -f *.o *.a
