UNITS_DIR=./build/test/units

units:
	mkdir -p $(UNITS_DIR)
	$(MAKE) -C ./libhttp/ units ARCH=$(ARCH)

clean:
	rm -rf $(UNITS_DIR)/*
	$(MAKE) -C ./libhttp/ clean

