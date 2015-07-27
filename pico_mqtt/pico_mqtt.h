/**
* Program settings
**/

#define WILDCARDS 0
#define SYSTEM_TOPICS 0
#define AUTO_RECONNECT 1

#define EMPTY_CLIENT_ID 0
#define LONG_CLIENT_ID 0
#define NON_ALPHA_NUMERIC_CLIENT_ID 0

#define MAXIMUM_MESSAGE_SIZE 500
#define MAXIMUM_TOPIC_SUBSCRIPTIONS 10
#define MAXIMUM_ACTIVE_MESSAGES 10
#define MAXIMUM_MEMORY_USE 2048

/**
* Public Function Prototypes
**/

// allocate memory for the pico_mqtt object and freeing it
struct pico_mqtt* pico_mqtt_create( const char* uri);
void pico_mqtt_destory(struct pico_mqtt** const mqtt);

// connect to the server and disconnect again
int pico_mqtt_connect( struct pico_mqtt* const mqtt, const uint32_t timeout);
void pico_mqtt_disconnect( struct pico_mqtt* const mqtt);

// ping the server and flush buffer and subscribtions (back to state after create)
void pico_mqtt_restart(struct pico_mqtt* const mqtt, const uint32_t timeout);
int pico_mqtt_ping( struct pico_mqtt* const mqtt, const uint32_t timeout );

// receive one or multiple messages or publis a message
int pico_mqtt_receive(struct pico_mqtt* const mqtt, uint32_t timeout);
void pico_mqtt_publish(struct pico_mqtt* const mqtt, const pico_mqtt_message message, const uint8_t quality_of_service, const uint32_t timeout);

//subscribe to a topic (or multiple topics) and unsubscribe
int pico_mqtt_subscribe(struct pico_mqtt* const mqtt, const char* topic, const uint8_t quality_of_service, const uint32_t timeout, const void (*callback)(struct pico_mqtt_message)); //return errors, qos is provided in message upon receive
void pico_mqtt_unsubscribe(struct pico_mqtt* const mqtt, const char* topic, const uint32_t timeout);

/**
* Helper Function Prototypes
**/

// free custom data types
void pico_mqtt_free_message(struct pico_mqtt_message * const message);
void pico_mqtt_free_data(struct pico_mqtt_data * const data);
void pico_mqtt_free_string(struct pico_mqtt_string * const string);

// create and destroy custom data types
pico_mqtt_string pico_mqtt_create_string(const char* string);
void pico_mqtt_destroy_string(pico_mqtt_string string);
pico_mqtt_data pico_mqtt_create_data(const void* data, const uint16_t length);
void pico_mqtt_destroy_data(pico_mqtt_data data);
pico_mqtt_message pico_mqtt_create_message(const char* topic, const uint8_t* data, const uint16_t length);
void pico_mqtt_destroy_message(pico_mqtt_message message);

// get the last error according to the documentation (MQTT specification)
const char* pico_mqtt_get_normative_error( const struct pico_mqtt* mqtt);
const uint32_t pico_mqtt_get_error_documentation_line( const struct pico_mqtt* mqtt);

// get the protocol version
const char* pico_mqtt_get_protocol_version( void );

/**
* Data Structures
**/

struct pico_mqtt_message{
	struct pico_mqtt_data data;
	char * topic;
	uint8_t duplicate			: 1;
	uint8_t quality_of_service	: 2;
	uint8_t retain				: 1;
	uint16_t message_id;
};

struct pico_mqtt_string{
	uint16_t length;
	char * string;
};

struct pico_mqtt_data{
	uint16_t length;
	void * data;
}