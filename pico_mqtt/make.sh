clear
gcc pico_mqtt_socket.c -Wall -Wextra -pedantic -Werror -Wno-cpp -c
gcc -Wall -Wextra -Werror -pedantic -Wno-cpp -std=c99 -c pico_mqtt_socket_test.c
gcc -o pico_mqtt_socket_test.out pico_mqtt_socket.o pico_mqtt_socket_test.o -lcheck -lpthread -lm -lrt
./pico_mqtt_socket_test.out
