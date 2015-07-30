#include "pico_mqtt.h"

/**
* Function Prototypes
**/

static uint8_t check_string_utf8( const pico_mqtt_string string );
//flags for wildcards (# and *), flag for null, flag for u+0000, flag for u+8000 < x < u+DFFFF
// flag for U+0001..U+001F control characters
// flag for U+007F..U+009F control characters
// flag for system topic

static uint8_t validate_fixed_header( const struct pico_mqtt_fixed_header header );
static uint32_t encode_variable_length( const uint32_t length );
static uint32_t decode_variable_length( const uint32_t length );
static uint8_t pico_mqtt_is_packet_id_required( const struct pico_mqtt_fixed_header* header);

/**
* Helper functions for debugging
**/

static void print_fixed_header( const struct pico_mqtt_fixed_header header, const uint8_t offset, const uint8_t starting_at);
static void print_connect_flags( const struct pico_mqtt_connect_flags, const uint8_t offset, const uint8_t starting_at);

/**
* Data Types
**/

static struct __attribute__((packed)) pico_mqtt_fixed_header{
	uint8_t packet_type 		: 4;
	uint8_t duplicate			: 1;
	uint8_t quality_of_service	: 2;
	uint8_t retain				: 1;
};

static struct pico_mqtt_connect_flags{
	uint8_t user_name	: 1;
	uint8_t password	: 1;
	uint8_t will_retain	: 1;
	uint8_t will_quality_of_service : 2;
	uint8_t will_flag	: 1;
	uint8_t clean_session	: 1;
	uint8_t reserved	: 1;
};

//not needed, 1 = 0x01
static struct pico_mqtt_connack_flags{
	uint8_t reserved	: 7;
	uint8_t session_present	: 1;
};

//not needed, 1 = 0x01
static struct pico_mqtt_subscribe_flags{
	uint8_t reserved	: 6;
	uint8_t quality_of_service	: 2;
};

static struct pico_mqtt_suback_flags{
	uint8_t failed	: 1;
	uint8_t reserved	: 5;
	uint8_t quality_of_service	: 2;
};

static struct pico_mqtt_message_wrapper{
	struct pico_mqtt_message* message;
	uint8_t status;
};

static struct pico_mqtt_topic{
	struct pico_mqtt_string name;
	uint8_t quality_of_service;
	void (*message_handler)(struct pico_mqtt_message);
};

static struct pico_mqtt{
	int socket;
	uint8_t tls_connections;
	char * URI;
	???* queue topics;
	struct pico_mqtt* this;

	// status
	uint8_t connected;
	char [15] normative_error;
	uint32_t documentation_reference;
	uint16_t next_packet_id;

	// connection related
	uint16_t keep_alive_time;
	struct pico_mqtt_string client_id;
	struct pico_mqtt_string will_topic;
	struct pico_mqtt_data will_message;
	struct pico_mqtt_string user_name;
	struct pico_mqtt_data password;
}
