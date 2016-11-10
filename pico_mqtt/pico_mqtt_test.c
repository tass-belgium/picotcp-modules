#define _BSD_SOURCE

#include <check.h>
#include <stdlib.h>
#include "pico_mqtt_list_mock.h"
#include "pico_mqtt_stream_mock.h"

#define DEBUG 3

/**
* test data types
**/

/**
* test functions prototypes
**/

Suite * mqtt_test_suite(void);
int compare_arrays(const void* a, const void* b, const uint32_t length);

/**
* file under test
**/

#include "pico_mqtt.c"

/**
* tests
**/

START(pico_mqtt_create_test)
{
	struct pico_mqtt* mqtt = NULL;

	mqtt = pico_mqtt_create();
	ck_assert_msg(mqtt != NULL, "Error while allocating memory for mqtt");
	pico_mqtt_stream_destroy(mqtt->stream);
	pico_mqtt_list_destroy(mqtt->output_queue);
	pico_mqtt_list_destroy(mqtt->wait_queue);
	pico_mqtt_list_destroy(mqtt->input_queue);
	FREE(mqtt);

	list_mock_set_create_fail( 1 );
	mqtt = pico_mqtt_create();
	ck_assert_msg(mqtt == NULL, "Error should have occurred when allocating the memory");

	list_mock_set_create_fail( 0 );
	stream_mock_set_create_fail( 1 );
	mqtt = pico_mqtt_create();
	ck_assert_msg(mqtt == NULL, "Error should have occurred when allocating the memory");

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	mqtt = pico_mqtt_create();
	ck_assert_msg(mqtt == NULL, "Error should have occurred when allocating the memory");
}
END

START(pico_mqtt_destroy_test)
{
	struct pico_mqtt* mqtt = NULL;

	pico_mqtt_destroy(NULL);

	mqtt = pico_mqtt_create();
	pico_mqtt_destroy(mqtt);
}
END

START(pico_mqtt_create_data_test)
{
	struct pico_mqtt_data* data = NULL;
	char string[]="Hello world!";

	data = pico_mqtt_create_data( string, 10 );
	ck_assert_msg(data != NULL, "message should be allocated.");
	ck_assert_msg(data->length == 10, "message should be 10 long.");
	FREE(data->data);
	FREE(data);
}
END

START(pico_mqtt_destroy_data_test)
{
	struct pico_mqtt_data* data = NULL;
	char string[]="Hello world!";

	pico_mqtt_destroy_data( data );

	data = pico_mqtt_create_data( NULL, 0 );
	ck_assert_msg(data != NULL, "Memory allocation error.");
	pico_mqtt_destroy_data( data );

	data = pico_mqtt_create_data( string , 1 );
	ck_assert_msg(data != NULL, "Memory allocation error.");

	pico_mqtt_destroy_data( data );
}
END

START(pico_mqtt_copy_data_test)
{
	struct pico_mqtt_data* data = NULL;
	struct pico_mqtt_data* copy = NULL;
	char string[]="Hello world!";

	copy = pico_mqtt_copy_data( data );
	ck_assert_msg(copy == NULL, "Incorrect copy of NULL message");

	data = pico_mqtt_create_data( NULL , 1 );
	copy = pico_mqtt_copy_data( data );
	ck_assert_msg(copy != NULL, "Copying the data should not fail");
	ck_assert_msg(copy->data == NULL, "The copied data should be NULL");
	ck_assert_msg(copy->length == 1, "The copied length should match");
	pico_mqtt_destroy_data( data );
	pico_mqtt_destroy_data( copy );

	data = pico_mqtt_create_data( string , 1 );
	*((uint8_t*)data->data) = 17;
	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	copy = pico_mqtt_copy_data( data );
	ck_assert_msg(copy == NULL, "Copying the data should fail");

	copy = pico_mqtt_copy_data( data );
	ck_assert_msg(copy != NULL, "Copying the data should not fail");
	ck_assert_msg(*((uint8_t*)copy->data) == 17, "The copied data should be 17");
	ck_assert_msg(copy->length == 1, "The copied length should match");

	pico_mqtt_destroy_data( data );
	pico_mqtt_destroy_data( copy );
}
END

START(pico_mqtt_copy_message_test)
{
	struct pico_mqtt_message* data = NULL;
	struct pico_mqtt_message* copy = NULL;

	copy = pico_mqtt_copy_message( data );
	ck_assert_msg(copy == NULL, "Incorrect copy of NULL message");

	data = pico_mqtt_create_message( "hello world", NULL , 0 );
	copy = pico_mqtt_copy_message( data );
	ck_assert_msg(copy != NULL, "Copying the data should not fail");
	ck_assert_msg(compare_arrays(copy->topic->data, "hello world", 11), "The topic is not set correctly");
	ck_assert_msg(copy->data->data == NULL, "The copied data should be NULL");
	ck_assert_msg(copy->data->length == 0, "The copied length should match");
	pico_mqtt_destroy_message( data );
	pico_mqtt_destroy_message( copy );

	data = pico_mqtt_create_message( "hello world", NULL , 0 );
	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	copy = pico_mqtt_copy_message( data );
	ck_assert_msg(copy == NULL, "Copying the data should fail");

	pico_mqtt_destroy_message( data );
	pico_mqtt_destroy_message( copy );
}
END

START(pico_mqtt_string_to_data_test)
{
	struct pico_mqtt_data* data = NULL;
	char string[13] = "Hello world!";

	data = pico_mqtt_string_to_data( NULL );
	ck_assert_msg(data != NULL, "Incorrect data packet");
	ck_assert_msg(data->length == 0, "The length is not correct");
	ck_assert_msg(data->data == NULL, "The data field should be NULL");
	pico_mqtt_destroy_data( data );

	data = pico_mqtt_string_to_data( string );
	ck_assert_msg(data != NULL, "Incorrect data packet");
	ck_assert_msg(data->length == 12, "The length is not correct");
	ck_assert_msg(compare_arrays((void*)string, data->data, data->length));
	pico_mqtt_destroy_data( data );
}
END

START(pico_mqtt_create_message_test)
{
	struct pico_mqtt_message* message = NULL;

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	message = pico_mqtt_create_message(NULL, NULL, 1);
	ck_assert_msg(message == NULL, "message should not be allocated.");

	message = pico_mqtt_create_message(NULL, NULL, 1);
	ck_assert_msg(message != NULL, "message should be allocated.");

	FREE(message->topic);
	FREE(message->data);
	FREE(message);
}
END

START(pico_mqtt_destroy_message_test)
{
	struct pico_mqtt_message* message = pico_mqtt_create_message(NULL, NULL, 1);
	ck_assert_msg(message != NULL, "message should be allocated.");

	pico_mqtt_destroy_message(NULL);
	pico_mqtt_destroy_message(message);
}
END

START(pico_mqtt_message_get_quality_of_service_test)
{
	struct pico_mqtt_message* message = NULL;
	uint8_t quality_of_service = 0;	
	message = pico_mqtt_create_message(NULL, NULL, 1);
	message->quality_of_service = 1;

	quality_of_service = pico_mqtt_message_get_quality_of_service(message);
	ck_assert_msg(quality_of_service == 1, "Quality_of_service should be 1.");

	message->quality_of_service = 2;
	quality_of_service = pico_mqtt_message_get_quality_of_service(message);
	ck_assert_msg(quality_of_service == 2, "Quality_of_service should be 2.");

	pico_mqtt_destroy_message( message );
}
END

START(pico_mqtt_message_set_quality_of_service_test)
{
	struct pico_mqtt_message* message = NULL;
	message = pico_mqtt_create_message(NULL, NULL, 1);
	message->quality_of_service = 0;

	pico_mqtt_message_set_quality_of_service(message, 1);
	ck_assert_msg(message->quality_of_service == 1, "Quality_of_service should be 1.");

	PERROR_DISABLE_ONCE();
	pico_mqtt_message_set_quality_of_service(message, 3);
	ck_assert_msg(message->quality_of_service == 1, "Quality_of_service should still be 1.");

	pico_mqtt_destroy_message( message );
}
END

START(pico_mqtt_message_get_retain_test)
{
	struct pico_mqtt_message* message = NULL;
	uint8_t retain = 0;	
	message = pico_mqtt_create_message(NULL, NULL, 1);

	message->retain = 1;
	retain = pico_mqtt_message_get_retain(message);
	ck_assert_msg(retain == 1, "retain should be 1.");

	message->retain = 0;
	retain = pico_mqtt_message_get_retain(message);
	ck_assert_msg(retain == 0, "retain should be 0.");

	pico_mqtt_destroy_message( message );
}
END

START(pico_mqtt_message_set_retain_test)
{
	struct pico_mqtt_message* message = NULL;
	message = pico_mqtt_create_message(NULL, NULL, 1);
	message->retain = 0;

	pico_mqtt_message_set_retain(message, 1);
	ck_assert_msg(message->retain == 1, "retain should be 1.");

	PERROR_DISABLE_ONCE();
	pico_mqtt_message_set_retain(message, 2);
	ck_assert_msg(message->retain == 1, "retain should still be 1.");

	pico_mqtt_destroy_message( message );
}
END

START(pico_mqtt_message_is_duplicate_flag_set_test)
{
	struct pico_mqtt_message* message = NULL;
	uint8_t duplicate = 0;
	message = pico_mqtt_create_message(NULL, NULL, 1);
	message->duplicate = 0;

	duplicate = pico_mqtt_message_is_duplicate_flag_set(message);
	ck_assert_msg(duplicate == 0, "duplicate should be 0.");

	message->duplicate = 1;
	duplicate = pico_mqtt_message_is_duplicate_flag_set(message);
	ck_assert_msg(duplicate == 1, "duplicate should be 1.");

	pico_mqtt_destroy_message( message );
}
END

START(pico_mqtt_message_get_message_id_test)
{
	struct pico_mqtt_message* message = NULL;
	uint16_t message_id = 0;
	message = pico_mqtt_create_message(NULL, NULL, 1);
	message->message_id = 1234;

	message_id = pico_mqtt_message_get_message_id(message);
	ck_assert_msg(message_id == 1234, "message_id should be 1234.");

	pico_mqtt_destroy_message( message );
}
END

START(pico_mqtt_message_get_topic_test)
{
	struct pico_mqtt_message* message = NULL;
	char hello_world[] = "Hello world!";
	char* string = NULL;

	message = pico_mqtt_create_message(NULL, NULL, 1);
	string = pico_mqtt_message_get_topic(message);
	ck_assert_msg(string == NULL, "The topic should be NULL.");
	FREE(message->topic);

	message->topic = NULL;
	string = pico_mqtt_message_get_topic(message);
	ck_assert_msg(string == NULL, "The topic should be NULL.");

	message->topic = pico_mqtt_string_to_data( hello_world );
	string = pico_mqtt_message_get_topic(message);
	ck_assert_msg(compare_arrays(hello_world, string, 13), "The topic should be Hello world! with \\0.");
	FREE(string);

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	string = pico_mqtt_message_get_topic(message);
	ck_assert_msg(string == NULL, "The topic should be NULL due to an error.");

	pico_mqtt_destroy_message( message );
}
END

START(is_valid_data_test)
{
	struct pico_mqtt_data data = PICO_MQTT_DATA_EMPTY;

	ck_assert_msg(is_valid_data(NULL), "Null is a valid data");
	ck_assert_msg(is_valid_data(&data) == 1, "An empty message should be valid");
	data.length = 1;
	ck_assert_msg(is_valid_data(&data) == 0, "A message with the length set and no data set is valid.");
	data.data = (void*) 1;
	ck_assert_msg(is_valid_data(&data) == 1, "Valid message should be recognised as such");
	data.length = 0;
	ck_assert_msg(is_valid_data(&data) == 0, "A message with no length should be invalid");
}
END

START(has_wildcards_test)
{
	struct pico_mqtt_data* topic = NULL;
	char no_wildcards[] = " !\"$%&\'()*,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
	char plus_wildcard[] = "hello + world!";
	char hash_wildcard[] = "#helloWorld!";

	topic = pico_mqtt_string_to_data(no_wildcards);
	ck_assert_msg(has_wildcards(topic) == 0, "The message does not contain any wildcards");
	pico_mqtt_destroy_data(topic);

	topic = pico_mqtt_string_to_data(plus_wildcard);
	ck_assert_msg(has_wildcards(topic) == 1, "The message does contain wildcards");
	pico_mqtt_destroy_data(topic);

	topic = pico_mqtt_string_to_data(NULL);
	ck_assert_msg(has_wildcards(topic) == 0, "The message does not contain any wildcards");
	pico_mqtt_destroy_data(topic);

	topic = pico_mqtt_string_to_data(hash_wildcard);
	ck_assert_msg(has_wildcards(topic) == 1, "The message does contain wildcards");
	pico_mqtt_destroy_data(topic);
}
END

#if ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0
START(has_only_alphanumeric_test)
{
	char all_alphnumeric_characters[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char invalid_string[] = "a+b=c";
	struct pico_mqtt_data* string = NULL;
	
	string = pico_mqtt_string_to_data(NULL);
	ck_assert_msg(has_only_alphanumeric(string) == 1, "The message does not contain any characters and is valid");
	pico_mqtt_destroy_data(string);

	string = pico_mqtt_string_to_data(all_alphnumeric_characters);
	ck_assert_msg(has_only_alphanumeric(string) == 1, "The message does contains only valid characters");
	pico_mqtt_destroy_data(string);

	string = pico_mqtt_string_to_data(invalid_string);
	ck_assert_msg(has_only_alphanumeric(string) == 0, "The message does contains invalid characters");
	pico_mqtt_destroy_data(string);
}
END
#endif //#if ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0

START(is_valid_unicode_string_test)
{
	char all_alphnumeric_characters[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char invalid_string[] = "hello world\x07";
	struct pico_mqtt_data* string = NULL;
	
	string = pico_mqtt_string_to_data(NULL);
	ck_assert_msg(is_valid_unicode_string(string) == 1, "An empty message is valid unicode");
	pico_mqtt_destroy_data(string);

	string = pico_mqtt_string_to_data(all_alphnumeric_characters);
	ck_assert_msg(is_valid_unicode_string(string) == 1, "The message does contains only valid characters");
	pico_mqtt_destroy_data(string);

	string = pico_mqtt_string_to_data(invalid_string);
	ck_assert_msg(is_valid_unicode_string(string) == 0, "The message does contains invalid characters");
	pico_mqtt_destroy_data(string);
}
END

START(is_system_topic_test)
{
	char valid_string[] = "0123456789abcdefghij$klm+nopqrs#tuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char invalid_string[] = "$hello world";
	struct pico_mqtt_data* string = NULL;
	
	string = pico_mqtt_string_to_data(NULL);
	ck_assert_msg(is_system_topic(string) == 0, "An empty message is no system topic");
	pico_mqtt_destroy_data(string);

	string = pico_mqtt_string_to_data(valid_string);
	ck_assert_msg(is_system_topic(string) == 0, "The string is no system topic");
	pico_mqtt_destroy_data(string);

	string = pico_mqtt_string_to_data(invalid_string);
	ck_assert_msg(is_system_topic(string) == 1, "The topic is a system topic");
	pico_mqtt_destroy_data(string);
}
END

START(pico_mqtt_set_client_id_test)
{
	int result = 0;
	struct pico_mqtt* mqtt = pico_mqtt_create();

	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_client_id(NULL, NULL);
	ck_assert_msg(result == ERROR, "An error should be generated");

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE();
	result = pico_mqtt_set_client_id(mqtt, "test");
	ck_assert_msg(result == ERROR, "An error should be generated");
	PERROR_ENABLE();

#if ALLOW_EMPTY_CLIENT_ID == 0
	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_client_id(mqtt, NULL);
	ck_assert_msg(result == ERROR, "An error should be generated");
#else
	result = pico_mqtt_set_client_id(mqtt, NULL);
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should be set");
	ck_assert_msg(mqtt->client_id->length == 0, "The client_id length should be 0");
	ck_assert_msg(mqtt->client_id->data == NULL, "The client_id data should be NULL");
#endif

	result = pico_mqtt_set_client_id(mqtt, "1");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should be set");
	ck_assert_msg(mqtt->client_id->length == 1, "The client_id length should be 1");
	ck_assert_msg(mqtt->client_id->data != NULL, "The client_id data should not be NULL");
	ck_assert_msg(*((char*)mqtt->client_id->data) == '1', "The client_id data should be 1");

	// check for memory leak when overriding the client id
	result = pico_mqtt_set_client_id(mqtt, "2");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should be set");
	ck_assert_msg(mqtt->client_id->length == 1, "The client_id length should be 1");
	ck_assert_msg(mqtt->client_id->data != NULL, "The client_id data should not be NULL");
	ck_assert_msg(*((char*)mqtt->client_id->data) == '2', "The client_id data should be 2");

#if ALLOW_LONG_CLIENT_ID == 0
	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_client_id(mqtt, "123456789123456789123456");
	ck_assert_msg(result == ERROR, "An error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should still be set");
	ck_assert_msg(mqtt->client_id->length == 1, "The client_id length should still be 1");
	ck_assert_msg(mqtt->client_id->data != NULL, "The client_id data should not be NULL");
	ck_assert_msg(*((char*)mqtt->client_id->data) == '2', "The client_id data should still be 2");
#else
	result = pico_mqtt_set_client_id(mqtt, "123456789123456789123456");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should be set");
	ck_assert_msg(mqtt->client_id->length == 24, "The client_id length should be 24");
	ck_assert_msg(mqtt->client_id->data != NULL, "The client_id data should not be NULL");
	ck_assert_msg(compare_arrays(mqtt->client_id->data, "123456789123456789123456", 24), "The client_id data should be 123456789123456789123456");
#endif //#if ALLOW_LONG_CLIENT_ID == 0

	result = pico_mqtt_set_client_id(mqtt, "2");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should be set");
	ck_assert_msg(mqtt->client_id->length == 1, "The client_id length should be 1");
	ck_assert_msg(mqtt->client_id->data != NULL, "The client_id data should not be NULL");
	ck_assert_msg(*((char*)mqtt->client_id->data) == '2', "The client_id data should be 2");

#if ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0
	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_client_id(mqtt, "hello_world!");
	ck_assert_msg(result == ERROR, "An error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should still be set");
	ck_assert_msg(mqtt->client_id->length == 1, "The client_id length should still be 1");
	ck_assert_msg(mqtt->client_id->data != NULL, "The client_id data should not be NULL");
	ck_assert_msg(*((char*)mqtt->client_id->data) == '2', "The client_id data should still be 2");
#else
	result = pico_mqtt_set_client_id(mqtt, "hello_world!");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should be set");
	ck_assert_msg(mqtt->client_id->length == 12, "The client_id length should be 12");
	ck_assert_msg(mqtt->client_id->data != NULL, "The client_id data should not be NULL");
	ck_assert_msg(compare_arrays(mqtt->client_id->data, "hello_world!", 12), "The client_id data should be hello_world!");

	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_client_id(mqtt, "hello world\x07");
	ck_assert_msg(result == ERROR, "An error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should still be set");
	ck_assert_msg(mqtt->client_id->length == 12, "The client_id length should still be 12");
	ck_assert_msg(mqtt->client_id->data != NULL, "The client_id data should not be NULL");
	ck_assert_msg(compare_arrays(mqtt->client_id->data, "hello_world!", 12), "The client_id data should still be hello_world!");
#endif //#if ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0

	pico_mqtt_destroy(mqtt);
}
END

START(pico_mqtt_unset_client_id_test)
{
	int result = 0;
	struct pico_mqtt* mqtt = pico_mqtt_create();


	result = pico_mqtt_set_client_id(mqtt, "1");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->client_id != NULL, "The client_id pointer should be set");
	ck_assert_msg(mqtt->client_id->length == 1, "The client_id length should be 1");
	ck_assert_msg(mqtt->client_id->data != NULL, "The client_id data should not be NULL");
	ck_assert_msg(*((char*)mqtt->client_id->data) == '1', "The client_id data should be 1");

	pico_mqtt_unset_client_id(mqtt);
	ck_assert_msg(mqtt->client_id == NULL, "The client_id should be NULL.");

	PWARNING_DISABLE_ONCE();
	pico_mqtt_unset_client_id(NULL);

	pico_mqtt_destroy(mqtt);
}
END

START(pico_mqtt_set_will_message_test)
{
	int result = 0;
	struct pico_mqtt* mqtt = pico_mqtt_create();
	struct pico_mqtt_message* message = NULL;

	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_will_message(mqtt, NULL);
	ck_assert_msg(result == ERROR, "An error should be generated");

	PERROR_DISABLE_ONCE();
	message = pico_mqtt_create_message("hello_world", NULL, 0);
	result = pico_mqtt_set_will_message(NULL, message);
	ck_assert_msg(result == ERROR, "An error should be generated");
	pico_mqtt_destroy_message(message);

	PERROR_DISABLE_ONCE();
	message = pico_mqtt_create_message("hello world\x07", NULL, 0);
	result = pico_mqtt_set_will_message(mqtt, message);
	ck_assert_msg(result == ERROR, "An error should be generated");
	ck_assert_msg(mqtt->will_message == NULL, "No will message should be set.\n");
	pico_mqtt_destroy_message(message);

	PERROR_DISABLE_ONCE();
	message = pico_mqtt_create_message("hello+world", NULL, 0);
	result = pico_mqtt_set_will_message(mqtt, message);
	ck_assert_msg(result == ERROR, "An error should be generated");
	ck_assert_msg(mqtt->will_message == NULL, "No will message should be set.\n");
	pico_mqtt_destroy_message(message);

	PERROR_DISABLE_ONCE();
	message = pico_mqtt_create_message("hello#world", NULL, 0);
	result = pico_mqtt_set_will_message(mqtt, message);
	ck_assert_msg(result == ERROR, "An error should be generated");
	ck_assert_msg(mqtt->will_message == NULL, "No will message should be set.\n");
	pico_mqtt_destroy_message(message);

	message = pico_mqtt_create_message("hello_world", NULL, 0);
	result = pico_mqtt_set_will_message(mqtt, message);
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->will_message != NULL, "the will message should be set.\n");
	ck_assert_msg(mqtt->will_message->topic->length == 11, "the will message topic should be 11 long\n");
	ck_assert_msg(compare_arrays(mqtt->will_message->topic->data, "hello_world", 11), "The topic is not set correctly");
	pico_mqtt_destroy_message(message);

	message = pico_mqtt_create_message("hello_world", NULL, 0);
	result = pico_mqtt_set_will_message(mqtt, message);
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->will_message != NULL, "the will message should be set.\n");
	ck_assert_msg(mqtt->will_message->topic->length == 11, "the will message topic should be 11 long\n");
	ck_assert_msg(compare_arrays(mqtt->will_message->topic->data, "hello_world", 11), "The topic is not set correctly");
	pico_mqtt_destroy_message(message);

	pico_mqtt_destroy(mqtt);
}
END

START(pico_mqtt_unset_will_message_test)
{
	int result = 0;
	struct pico_mqtt* mqtt = pico_mqtt_create();
	struct pico_mqtt_message* message = pico_mqtt_create_message("hello_world", NULL, 0);


	result = pico_mqtt_set_will_message(mqtt, message);
	pico_mqtt_destroy_message(message);
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->will_message != NULL, "the will message should be set.\n");
	ck_assert_msg(mqtt->will_message->topic->length == 11, "the will message topic should be 11 long\n");
	ck_assert_msg(compare_arrays(mqtt->will_message->topic->data, "hello_world", 11), "The topic is not set correctly");

	pico_mqtt_unset_will_message(mqtt);
	ck_assert_msg(mqtt->will_message == NULL, "The will_message should be NULL.");

	PWARNING_DISABLE_ONCE();
	pico_mqtt_unset_will_message(NULL);

	pico_mqtt_destroy(mqtt);
}
END

START(pico_mqtt_set_username_test)
{
	int result = 0;
	struct pico_mqtt* mqtt = pico_mqtt_create();

	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_username(NULL, NULL);
	ck_assert_msg(result == ERROR, "An error should be generated");

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE();
	result = pico_mqtt_set_username(mqtt, "test");
	ck_assert_msg(result == ERROR, "An error should be generated");
	PERROR_ENABLE();

#if ALLOW_EMPTY_USERNAME == 0
	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_username(mqtt, NULL);
	ck_assert_msg(result == ERROR, "An error should be generated");
#else
	result = pico_mqtt_set_username(mqtt, NULL);
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->username != NULL, "The username pointer should be set");
	ck_assert_msg(mqtt->username->length == 0, "The username length should be 0");
	ck_assert_msg(mqtt->username->data == NULL, "The username data should be NULL");
#endif

	result = pico_mqtt_set_username(mqtt, "1");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->username != NULL, "The username pointer should be set");
	ck_assert_msg(mqtt->username->length == 1, "The username length should be 1");
	ck_assert_msg(mqtt->username->data != NULL, "The username data should not be NULL");
	ck_assert_msg(*((char*)mqtt->username->data) == '1', "The username data should be 1");

	// check for memory leak when overriding the username
	result = pico_mqtt_set_username(mqtt, "2");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->username != NULL, "The username pointer should be set");
	ck_assert_msg(mqtt->username->length == 1, "The username length should be 1");
	ck_assert_msg(mqtt->username->data != NULL, "The username data should not be NULL");
	ck_assert_msg(*((char*)mqtt->username->data) == '2', "The username data should be 2");

	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_username(mqtt, "hello world\x07");
	ck_assert_msg(result == ERROR, "An error should be generated");
	ck_assert_msg(mqtt->username != NULL, "The username pointer should be set");
	ck_assert_msg(mqtt->username->length == 1, "The username length should still be 1");
	ck_assert_msg(mqtt->username->data != NULL, "The username data should not be NULL");
	ck_assert_msg(*((char*)mqtt->username->data) == '2', "The username data should strill be 2");

	pico_mqtt_destroy(mqtt);
}
END

START(pico_mqtt_unset_username_test)
{
	int result = 0;
	struct pico_mqtt* mqtt = pico_mqtt_create();

	result = pico_mqtt_set_username(mqtt, "1");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->username != NULL, "The username pointer should be set");
	ck_assert_msg(mqtt->username->length == 1, "The username length should be 1");
	ck_assert_msg(mqtt->username->data != NULL, "The username data should not be NULL");
	ck_assert_msg(*((char*)mqtt->username->data) == '1', "The username data should be 1");

	pico_mqtt_unset_username(mqtt);
	ck_assert_msg(mqtt->username == NULL, "The username should be NULL.");

	PWARNING_DISABLE_ONCE();
	pico_mqtt_unset_username(NULL);

	pico_mqtt_destroy(mqtt);
}
END

START(pico_mqtt_set_password_test)
{
	int result = 0;
	struct pico_mqtt* mqtt = pico_mqtt_create();

	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_password(NULL, NULL);
	ck_assert_msg(result == ERROR, "An error should be generated");

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE();
	result = pico_mqtt_set_password(mqtt, "test");
	ck_assert_msg(result == ERROR, "An error should be generated");
	PERROR_ENABLE();

	result = pico_mqtt_set_password(mqtt, NULL);
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->password != NULL, "The password pointer should be set");
	ck_assert_msg(mqtt->password->length == 0, "The password length should be 0");
	ck_assert_msg(mqtt->password->data == NULL, "The password data should be NULL");

	result = pico_mqtt_set_password(mqtt, "1");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->password != NULL, "The password pointer should be set");
	ck_assert_msg(mqtt->password->length == 1, "The password length should be 1");
	ck_assert_msg(mqtt->password->data != NULL, "The password data should not be NULL");
	ck_assert_msg(*((char*)mqtt->password->data) == '1', "The password data should be 1");

	// check for memory leak when overriding the password
	result = pico_mqtt_set_password(mqtt, "2");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->password != NULL, "The password pointer should be set");
	ck_assert_msg(mqtt->password->length == 1, "The password length should be 1");
	ck_assert_msg(mqtt->password->data != NULL, "The password data should not be NULL");
	ck_assert_msg(*((char*)mqtt->password->data) == '2', "The password data should be 2");

	PERROR_DISABLE_ONCE();
	result = pico_mqtt_set_password(mqtt, "hello world\x07");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->password != NULL, "The password pointer should be set");
	ck_assert_msg(mqtt->password->length == 12, "The password length should be 12");
	ck_assert_msg(mqtt->password->data != NULL, "The password data should not be NULL");
	ck_assert_msg(compare_arrays(mqtt->password->data, "hello world\x07", 12), "The password data should strill be hello world\x07");

	pico_mqtt_destroy(mqtt);
}
END

START(pico_mqtt_unset_password_test)
{
	int result = 0;
	struct pico_mqtt* mqtt = pico_mqtt_create();

	result = pico_mqtt_set_password(mqtt, "1");
	ck_assert_msg(result == SUCCES, "No error should be generated");
	ck_assert_msg(mqtt->password != NULL, "The password pointer should be set");
	ck_assert_msg(mqtt->password->length == 1, "The password length should be 1");
	ck_assert_msg(mqtt->password->data != NULL, "The password data should not be NULL");
	ck_assert_msg(*((char*)mqtt->password->data) == '1', "The password data should be 1");

	pico_mqtt_unset_password(mqtt);
	ck_assert_msg(mqtt->password == NULL, "The password should be NULL.");

	PWARNING_DISABLE_ONCE();
	pico_mqtt_unset_password(NULL);

	pico_mqtt_destroy(mqtt);
}
END

START(is_valid_topic_test)
{
	struct pico_mqtt* mqtt = pico_mqtt_create();
	struct pico_mqtt_data* topic = NULL;

	PERROR_DISABLE_ONCE();
	ck_assert_msg(is_valid_topic(mqtt, NULL) == 0, "An error should be generated");

	topic = pico_mqtt_string_to_data("hello_world");
	ck_assert_msg(is_valid_topic(NULL, topic) == 1, "No error should be generated");
	pico_mqtt_destroy_data(topic);

	PERROR_DISABLE_ONCE();
	topic = pico_mqtt_string_to_data(NULL);
	ck_assert_msg(is_valid_topic(mqtt, topic) == 0, "An error should be generated");
	pico_mqtt_destroy_data(topic);

	topic = pico_mqtt_string_to_data("hello_#world+");
	ck_assert_msg(is_valid_topic(NULL, topic) == 1, "No error should be generated");
	pico_mqtt_destroy_data(topic);

#if ALLOW_SYSTEM_TOPICS == 0
	PERROR_DISABLE_ONCE();
	topic = pico_mqtt_string_to_data("$hello_world");
	ck_assert_msg(is_valid_topic(NULL, topic) == 0, "An error should be generated");
	pico_mqtt_destroy_data(topic);
#else
	topic = pico_mqtt_string_to_data("$hello_world");
	ck_assert_msg(is_valid_topic(NULL, topic) == 1, "No error should be generated");
	pico_mqtt_destroy_data(topic);
#endif //#if ALLOW_SYSTEM_TOPICS == 0

	PERROR_DISABLE_ONCE();
	topic = pico_mqtt_string_to_data("hello world\x07");
	ck_assert_msg(is_valid_topic(mqtt, topic) == 0, "An error should be generated");
	pico_mqtt_destroy_data(topic);

	pico_mqtt_destroy(mqtt);
}
END

START(is_quality_of_service_valid_test)
{
	struct pico_mqtt* mqtt = pico_mqtt_create();

	ck_assert_msg(is_quality_of_service_valid(NULL, 2) == 1, "The quality_of_service should be valid");
	ck_assert_msg(is_quality_of_service_valid(NULL, 1) == 1, "The quality_of_service should be valid");
	ck_assert_msg(is_quality_of_service_valid(NULL, 0) == 1, "The quality_of_service should be valid");

	PERROR_DISABLE_ONCE();
	ck_assert_msg(is_quality_of_service_valid(NULL, 3) == 0, "The quality_of_service should be invalid");

	PERROR_DISABLE_ONCE();
	ck_assert_msg(is_quality_of_service_valid(mqtt, 3) == 0, "The quality_of_service should be valid");

	pico_mqtt_destroy(mqtt);
}
END

START(pico_mqtt_set_keep_alive_time_test)
{
	struct pico_mqtt* mqtt = pico_mqtt_create();

	pico_mqtt_set_keep_alive_time(mqtt, 1234);
	ck_assert_msg(mqtt->keep_alive_time == 1234, "The keep_alive_time is not set correctly.");

	pico_mqtt_destroy(mqtt);
}
END

Suite * mqtt_test_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("MQTT serializer");

	// Core test case
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, pico_mqtt_create_test);
	tcase_add_test(test_case_core, pico_mqtt_destroy_test);
	tcase_add_test(test_case_core, pico_mqtt_create_data_test);
	tcase_add_test(test_case_core, pico_mqtt_destroy_data_test);
	tcase_add_test(test_case_core, pico_mqtt_copy_data_test);
	tcase_add_test(test_case_core, pico_mqtt_copy_message_test);
	tcase_add_test(test_case_core, pico_mqtt_string_to_data_test);
	tcase_add_test(test_case_core, pico_mqtt_create_message_test);
	tcase_add_test(test_case_core, pico_mqtt_destroy_message_test);
	tcase_add_test(test_case_core, pico_mqtt_message_get_quality_of_service_test);
	tcase_add_test(test_case_core, pico_mqtt_message_set_quality_of_service_test);
	tcase_add_test(test_case_core, pico_mqtt_message_get_retain_test);
	tcase_add_test(test_case_core, pico_mqtt_message_set_retain_test);
	tcase_add_test(test_case_core, pico_mqtt_message_is_duplicate_flag_set_test);
	tcase_add_test(test_case_core, pico_mqtt_message_get_message_id_test);
	tcase_add_test(test_case_core, pico_mqtt_message_get_topic_test);
	tcase_add_test(test_case_core, is_valid_data_test);
	tcase_add_test(test_case_core, has_wildcards_test);
#if ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0
	tcase_add_test(test_case_core, has_only_alphanumeric_test);
#endif //#if ALLOW_NON_ALPHNUMERIC_CLIENT_ID == 0
	tcase_add_test(test_case_core, is_valid_unicode_string_test);
	tcase_add_test(test_case_core, is_system_topic_test);
	tcase_add_test(test_case_core, pico_mqtt_set_client_id_test);
	tcase_add_test(test_case_core, pico_mqtt_unset_client_id_test);
	tcase_add_test(test_case_core, pico_mqtt_set_will_message_test);
	tcase_add_test(test_case_core, pico_mqtt_unset_will_message_test);
	tcase_add_test(test_case_core, pico_mqtt_set_username_test);
	tcase_add_test(test_case_core, pico_mqtt_unset_username_test);
	tcase_add_test(test_case_core, pico_mqtt_set_password_test);
	tcase_add_test(test_case_core, pico_mqtt_unset_password_test);
	tcase_add_test(test_case_core, is_valid_topic_test);
	tcase_add_test(test_case_core, is_quality_of_service_valid_test);
	tcase_add_test(test_case_core, pico_mqtt_set_keep_alive_time_test);
//	tcase_add_test(test_case_core, pico_mqtt_set_keep_alive_time_test);

	suite_add_tcase(test_suite, test_case_core);

	return test_suite;
}

 int main(void)
 {
	int number_failed;
	Suite *test_suite;
	SRunner *suite_runner;

	test_suite = mqtt_test_suite();
	suite_runner = srunner_create(test_suite);

	srunner_run_all(suite_runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(suite_runner);
	srunner_free(suite_runner);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }

 int compare_arrays(const void* a, const void* b, const uint32_t length)
{
	uint32_t index = 0;
	for (index = 0; index < length; ++index)
	{
		//printf("--- %c - %c\n", *((const char*)a + index), *((const char*)b + index));
		if(*((const uint8_t*)a + index) != *((const uint8_t*)b + index))
			return 0;
	}

	return 1;
}
