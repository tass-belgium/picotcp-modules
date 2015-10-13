all: pico_mqtt_socket_test.out pico_mqtt_stream_test.out pico_mqtt_list_test.out demo.out

pico_mqtt_test.out: pico_mqtt.c pico_mqtt_test.c
	gcc -Wall -Wextra -Werror -o pico_mqtt_test.out pico_mqtt_test.c -lcheck -lpthread -lm -lrt
	./pico_mqtt_test.out

#pico_mqtt_socket_test.out: pico_mqtt_socket.c pico_mqtt_socket_test.c
#	gcc -Wall -Wextra -Werror -std=c99 -o pico_mqtt_socket_test.out pico_mqtt_socket_test.c -lcheck -lpthread -lm -lrt

pico_mqtt_socket_test.out: pico_mqtt_socket.c pico_mqtt_socket_test.c
	gcc -Wall -o pico_mqtt_socket_test.out pico_mqtt_socket_test.c -lcheck -lpthread -lm -lrt
	./pico_mqtt_socket_test.out

pico_mqtt_stream_test.out: pico_mqtt_stream.c pico_mqtt_stream_test.c
	gcc -Wall -Wextra -Werror -o pico_mqtt_stream_test.out pico_mqtt_stream_test.c -lcheck -lpthread -lm -lrt
	./pico_mqtt_stream_test.out

pico_mqtt_list_test.out: pico_mqtt_list.c pico_mqtt_list_test.c
	gcc -Wall -Wextra -Werror -o pico_mqtt_list_test.out pico_mqtt_list_test.c -lcheck -lpthread -lm -lrt

demo.out: demo.c pico_mqtt.c pico_mqtt_list.c pico_mqtt_stream.c pico_mqtt_socket.c pico_mqtt_serializer.c
	gcc -Wall -Wextra -Werror -o demo.out demo.c pico_mqtt.c pico_mqtt_list.c pico_mqtt_stream.c pico_mqtt_socket.c pico_mqtt_serializer.c -DDEBUG=3
	./demo.out