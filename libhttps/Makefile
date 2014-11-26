-include ../../../config.mk
-include ../../../tools/kconfig/.config

PREFIX?=$(PWD)/build
CROSS_COMPILE?=arm-none-eabi-
CC?=gcc
ARCH?=stm32
CXX_FILES := $(wildcard *.c)
OBJS:= $(patsubst %.c,%.o,$(CXX_FILES))
CFLAGS+=-Iconfig $(EXTRA_CFLAGS) $(PLATFORM_CFLAGS) -I $(PREFIX)/include 

all: $(PREFIX)/lib/libhttps.a

CFLAGS-y:=
CFLAGS-$(CYASSL)+=-DLIBHTTPS_USE_CYASSL -DCYASSL_PICOTCP -DCYASSL_USER_IO -DNO_WRITEV
CFLAGS-$(POLARSSL)+=-DLIBHTTPS_USE_POLARSSL

CFLAGS+=$(CFLAGS-y)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(PREFIX)/lib/libhttps.a: $(OBJS)
	cp *.h $(PREFIX)/include
	@$(CROSS_COMPILE)ar cru $@ $(OBJS)
	@$(CROSS_COMPILE)ranlib $@

clean:
	rm -f $(OBJS)
