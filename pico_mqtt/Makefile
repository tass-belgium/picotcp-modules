CC=gcc
CC_FLAGS= -Wall -Wdeclaration-after-statement -W -Wextra -Wshadow -Wcast-qual -Wwrite-strings -Wunused-variable -Wundef -Wunused-function -Wconversion -Wcast-align -Wmissing-prototypes -Wno-missing-field-initializers -Werror
TEST_FLAGS= -lcheck -lpthread -lm -lrt

#Main target
tests: serializer_test
#serializer: serializer_test.o
#	$(CC) serializer_test.o -o serializer

#To obtain object files
serializer.o: pico_mqtt_serializer.c pico_mqtt_serializer.h
	$(CC) $(CC_FLAGS) -c pico_mqtt_serializer.c -o serializer.o

serializer_test: pico_mqtt_serializer_test.c pico_mqtt_serializer.c pico_mqtt_serializer.h
	$(CC) $(CC_FLAGS) pico_mqtt_serializer_test.c -o serializer_test.out $(TEST_FLAGS)
	./serializer_test.out

#To remove generated files
clean:
	rm -f serializer_test.out serializer.o