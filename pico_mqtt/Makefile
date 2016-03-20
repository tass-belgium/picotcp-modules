CC=gcc
CC_FLAGS= -Wdeclaration-after-statement -W -Wshadow -Wcast-qual -Wwrite-strings -Wundef -Wconversion -Wcast-align -Wmissing-prototypes -Wno-missing-field-initializers -Wunused-function -Wextra -Werror -Wunused-variable -Wall
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
	complexity --score --thresh=3 pico_mqtt_serializer.c
	gcov pico_mqtt_serializer_test.c

list_test: pico_mqtt_list_test.c pico_mqtt_list.c pico_mqtt_list.h
	$(CC) $(CC_FLAGS) pico_mqtt_list_test.c -o list_test.out $(TEST_FLAGS)
	./list_test.out
	complexity --score --thresh=2 pico_mqtt_list.c
	gcov pico_mqtt_list_test.c

#To remove generated files
clean:
	rm -f *.out *.o *.gcov *.gcda *.gcno