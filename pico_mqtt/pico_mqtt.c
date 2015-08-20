#include "pico_mqtt.h"

/**
* Data Types
**/

struct pico_mqtt_string{
	uint32_t length;
	char* string;
};

struct __attribute__((packed)) pico_mqtt_fixed_header{
	uint8_t packet_type 		: 4;
	uint8_t duplicate			: 1;
	uint8_t quality_of_service	: 2;
	uint8_t retain				: 1;
};

struct pico_mqtt_connect_flags{
	uint8_t user_name	: 1;
	uint8_t password	: 1;
	uint8_t will_retain	: 1;
	uint8_t will_quality_of_service : 2;
	uint8_t will_flag	: 1;
	uint8_t clean_session	: 1;
	uint8_t reserved	: 1;
};

//not needed, 1 = 0x01
struct pico_mqtt_connack_flags{
	uint8_t reserved	: 7;
	uint8_t session_present	: 1;
};

//not needed, 1 = 0x01
struct pico_mqtt_subscribe_flags{
	uint8_t reserved	: 6;
	uint8_t quality_of_service	: 2;
};

struct pico_mqtt_suback_flags{
	uint8_t failed	: 1;
	uint8_t reserved	: 5;
	uint8_t quality_of_service	: 2;
};

struct pico_mqtt_message_wrapper{
	struct pico_mqtt_message* message;
	uint8_t status;
};

struct pico_mqtt_topic{
	struct pico_mqtt_string name;
	uint8_t quality_of_service;
	void (*message_handler)(struct pico_mqtt_message);
};

struct pico_mqtt{
	int socket;
	uint8_t tls_connections;
	char * URI;
	struct pico_mqtt* this;

	// status
	uint8_t connected;
	char normative_error[15];
	uint32_t documentation_reference;
	uint16_t next_packet_id;

	// connection related
	uint16_t keep_alive_time;
	struct pico_mqtt_string client_id;
	struct pico_mqtt_string will_topic;
	struct pico_mqtt_data will_message;
	struct pico_mqtt_string user_name;
	struct pico_mqtt_data password;

	// socket write status
	uint32_t output_message_next_byte;
	const struct pico_mqtt_data* output_message;
};

/**
* Function Prototypes
**/

static inline void initialize(struct pico_mqtt* const mqtt);
static inline uint8_t is_time_left(const struct timeval* const time_left);
static inline uint8_t is_output_ready(const struct pico_mqtt* mqtt);
static int set_output_message( struct pico_mqtt * mqtt, const struct pico_mqtt_data* serialized_message);
static uint32_t encode_variable_length( const uint32_t length );
static uint32_t decode_variable_length( const uint32_t length );

/**
* Helper functions for debugging
**/

static void print_fixed_header( const struct pico_mqtt_fixed_header header, const uint8_t offset, const uint8_t starting_at);
static void print_connect_flags( const struct pico_mqtt_connect_flags, const uint8_t offset, const uint8_t starting_at);

/**
* public function implementation
**/

struct pico_mqtt* pico_mqtt_create( const char* uri, uint32_t timeout){
	struct pico_mqtt* mqtt = (struct pico_mqtt *) malloc(sizeof(struct pico_mqtt)); /*//TODO check if memory was allocated */
	initialize( mqtt);
	return mqtt;
}

/**
* private function implementation
**/

static inline void initialize(struct pico_mqtt* const mqtt){
	*mqtt = (struct pico_mqtt) {
		.output_message_next_byte = 0,
		.output_message = NULL
	};
}

static inline uint8_t is_time_left(const struct timeval* const time_left){
	return (time_left->tv_sec > 0) || (time_left->tv_usec > 0);
}

static inline uint8_t is_output_ready(const struct pico_mqtt* mqtt){
	return mqtt->output_message == NULL;
}

/**
* set new output message
*
* return 0 if the previous message was not yet compleet
* return 1 for succes
**/

static int set_output_message( struct pico_mqtt * mqtt, const struct pico_mqtt_data* serialized_message){
	if(!is_output_ready(mqtt)){
		return 0;
	} else {
		mqtt->output_message = serialized_message;
		mqtt->output_message_next_byte = 0;
		return 1;
	}
} 

/**
* tries to read and write messages
*
* call callbacks in case of error or function ready
**/

static int read_write_messages( struct pico_mqtt* mqtt, struct timeval* time_left){
/*	while(*next_byte < message->length){
		if(time_left != NULL){
			if( !is_time_left(time_left) )
				return 0;
		}
		
//		result = pico_mqtt_connection_read_write( mqtt->socket, message->data + *next_byte, message->length - *next_byte, time_left);

		if( result < 0 )
			return -1;

		*next_byte += result;
	}

	return 1;
*/
return 1;
}
