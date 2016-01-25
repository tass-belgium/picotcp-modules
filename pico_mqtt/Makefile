CC=gcc
CC_FLAGS= -Wall -Wdeclaration-after-statement -W -Wextra -Wshadow -Wcast-qual -Wwrite-strings -Wunused-variable -Wundef -Wunused-function -Wconversion -Wcast-align -Wmissing-prototypes -Wno-missing-field-initializers -Werror
TEST_FLAGS= -lcheck -lpthread -lm -lrt -fprofile-arcs -ftest-coverage

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
	gcov pico_mqtt_serializer_test.c
	complexity --score --thresh=3 pico_mqtt_serializer.c

#To remove generated files
clean:
	rm -f *.out *.o *.gcov *.gcda *.gcno