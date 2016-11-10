#ifndef PICO_MQTT_CONFIGURATION_H
#define PICO_MQTT_CONFIGURATION_H
/**
* Library Configuration
**/

/* set to production mode to strip all macro's and debug related code */
/* #define PRODUCTION */
#define AUTO_RECONNECT 1
#define MAXIMUM_RECONNECTION_ATTEMPTS 3

#define ALLOW_WILDCARDS 0
#define ALLOW_SYSTEM_TOPICS 0

#define ALLOW_LONG_CLIENT_ID 0
#define ALLOW_EMPTY_CLIENT_ID 0
#define ALLOW_NON_ALPHNUMERIC_CLIENT_ID 0
#define ALLOW_EMPTY_USERNAME 0
#define REQUIRE_USERNAME 0
#define REQUIRE_PASSWORD 0

#define MAXIMUM_MESSAGE_SIZE 500

#define ENBABLE_DNS_LOOKUP 1

#endif