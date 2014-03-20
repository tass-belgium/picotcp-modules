all: libhttp/libhttp.a

libhttp/libhttp.a:
	make -C libhttp ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) FEATURES=$(FEATURES)


clean:
	make -C libhttp clean
	rm -rf picotcp
