CC=gcc
CC_FLAGS= -Wdeclaration-after-statement -W -Wshadow -Wcast-qual -Wwrite-strings -Wundef -Wconversion -Wcast-align -Wmissing-prototypes -Wno-missing-field-initializers -Wunused-function -Wextra -Werror -Wunused-variable -Wall
TEST_FLAGS= -lcheck -lpthread -lm -lrt -fprofile-arcs -ftest-coverage -lsubunit
COMPLEXITY_THRESHOLD= 3
EXTENSION= out

#Main target
tests: serializer_test
#serializer: serializer_test.o
#	$(CC) serializer_test.o -o serializer

#To obtain object files

debug.o: pico_mqtt_debug.c pico_mqtt_debug.h
	$(CC) $(CC_FLAGS) -c pico_mqtt_debug.c -o $@

serializer.o: pico_mqtt_serializer.c pico_mqtt_serializer.h
	$(CC) $(CC_FLAGS) -c pico_mqtt_serializer.c -o $@

serializer_mock.o: pico_mqtt_serializer_mock.c pico_mqtt_serializer.h pico_mqtt_serializer_mock.h
	$(CC) $(CC_FLAGS) -c pico_mqtt_serializer_mock.c -o $@

socket_mock.o: pico_mqtt_socket.h pico_mqtt_socket_mock.c pico_mqtt_socket_mock.h
	$(CC) $(CC_FLAGS) -c pico_mqtt_socket_mock.c -o $@

serializer_test: pico_mqtt_serializer_test.c pico_mqtt_serializer.c pico_mqtt_serializer.h debug.o
	$(CC) $(CC_FLAGS) pico_mqtt_$(basename $@).c $(filter %.o,$^) -o $@.$(EXTENSION) $(TEST_FLAGS)
	./$@.$(EXTENSION)
	complexity --score --thresh=$(COMPLEXITY_THRESHOLD) pico_mqtt_serializer.c
	gcov pico_mqtt_serializer_test.c

list_test: pico_mqtt_list_test.c pico_mqtt_list.c pico_mqtt_list.h debug.o
	$(CC) $(CC_FLAGS) pico_mqtt_$(basename $@).c $(filter %.o,$^) -o $@.$(EXTENSION) $(TEST_FLAGS)
	./$@.$(EXTENSION)
	complexity --score --thresh=$(COMPLEXITY_THRESHOLD) pico_mqtt_list.c
	gcov pico_mqtt_list_test.c

stream_test: pico_mqtt_stream_test.c pico_mqtt_stream.c pico_mqtt_stream.h socket_mock.o serializer_mock.o debug.o
	$(CC) $(CC_FLAGS) pico_mqtt_$(basename $@).c $(filter %.o,$^) -o $@.$(EXTENSION) $(TEST_FLAGS)
	./$@.$(EXTENSION)
	complexity --score --thresh=$(COMPLEXITY_THRESHOLD) pico_mqtt_stream.c
	gcov pico_mqtt_$(basename $@).c

#To remove generated files
clean:
	rm -f *.$(EXTENSION) *.o *.gcov *.gcda *.gcno *.h.gch
