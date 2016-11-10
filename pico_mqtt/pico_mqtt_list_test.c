#define _BSD_SOURCE

#include <check.h>
#include <stdlib.h>

#define DEBUG 3

/**
* test data types
**/

/**
* test functions prototypes
**/

int compare_arrays(void* a, void* b, uint32_t length);
Suite * functional_list_suite(void);

/**
* file under test
**/

#include "pico_mqtt_list.c"

/**
* tests
**/

START_TEST(compare_arrays_test)
{
	uint8_t a1[5] = {1, 2, 3, 4, 5};
	uint8_t a2[5] = {1, 2, 4, 4, 5};

	ck_assert_msg(compare_arrays(a1, a1, 5), "The arrays should match.\n");
	ck_assert_msg(compare_arrays(a2, a2, 5), "The arrays should match.\n");
	ck_assert_msg(!compare_arrays(a1, a2, 5), "The arrays should not match.\n");
	ck_assert_msg(!compare_arrays(a2, a1, 5), "The arrays should not match.\n");

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(debug_malloc_test)
{
	void* result = NULL;

	MALLOC_SUCCEED();
	result = MALLOC(1);
	ck_assert_msg(result != NULL, "Malloc should not have failed\n");
	FREE(result);

	MALLOC_FAIL();
	result = MALLOC(1);
	ck_assert_msg(result == NULL, "Malloc should have failed\n");

	result = MALLOC(1);
	ck_assert_msg(result == NULL, "Malloc should have failed\n");

	MALLOC_FAIL_ONCE();
	result = MALLOC(1);
	ck_assert_msg(result == NULL, "Malloc should have failed\n");

	result = MALLOC(1);
	ck_assert_msg(result != NULL, "Malloc not should have failed\n");
	FREE(result);


	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_list_create_test)
{
	int error = 0;
	struct pico_mqtt_list* list = NULL;

	MALLOC_SUCCEED();
	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed\n");
	ck_assert_msg(list->first == NULL, "The first elements should be NULL.\n");
	ck_assert_msg(list->last == NULL, "The last elements should be NULL.\n");
	FREE(list);

	PERROR_DISABLE_ONCE();
	MALLOC_FAIL_ONCE();

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list == NULL, "Creating the list should have failed\n");

	ALLOCATION_REPORT();
	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(create_element_test)
{
	struct pico_mqtt_packet packet = PICO_MQTT_PACKET_EMPTY;
	struct element* element = NULL;

	MALLOC_SUCCEED();
	element = create_element( &packet );
	ck_assert_msg(element != NULL, "Creating the element should not have failed\n");
	ck_assert_msg(element->packet == &packet, "The pointer to the packet should be set correctly.\n");
	ck_assert_msg(element->previous == NULL, "The pointer to the previous element should have been set to NULL.\n");
	ck_assert_msg(element->next == NULL, "The pointer to the next element should have been set to NULL.\n");
	FREE(element);

	MALLOC_FAIL_ONCE();
	PERROR_DISABLE_ONCE();
	element = create_element( &packet );
	ck_assert_msg(element == NULL, "Creating the element should have failed\n");

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_list_length_test)
{
	int error = 0;
	struct pico_mqtt_list* list = NULL;

	MALLOC_SUCCEED();
	list = pico_mqtt_list_create( &error );
	ck_assert_msg(pico_mqtt_list_length(list) == 0, "The length is not correct.\n");
	list->length = 123;
	ck_assert_msg(pico_mqtt_list_length(list) == 123, "The length is not correct.\n");

	pico_mqtt_list_destroy( list );

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(destroy_element_test)
{
	struct pico_mqtt_packet packet = PICO_MQTT_PACKET_EMPTY;
	struct element* element = NULL;

	MALLOC_SUCCEED();
	element = create_element( &packet );
	ck_assert_msg(element != NULL, "Creating the element should not have failed\n");

	destroy_element(element);

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mttq_list_destroy_test)
{
	struct element* element = NULL;
	struct pico_mqtt_list* list = NULL;
	int error = 0;

	MALLOC_SUCCEED();

/*
	NO LIST
*/
	pico_mqtt_list_destroy(NULL);

/*
	EMPTY LIST
*/
	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed\n");
	list->length = 0;
	pico_mqtt_list_destroy(list);
	CHECK_NO_ALLOCATIONS();

/*
	1 ELEMENT
*/

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed\n");

	element = create_element( (struct pico_mqtt_packet*) MALLOC(sizeof(struct pico_mqtt_packet)) );
	ck_assert_msg(element != NULL, "Creating the element should not have failed\n");
	list->first = element;
	element->previous = NULL;
	element->next = NULL;
	list->last = element;

	list->length = 1;

	pico_mqtt_list_destroy(list);

	CHECK_NO_ALLOCATIONS();

/*
	MULTIPLE ELEMENTS
*/

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed\n");

	MALLOC_SUCCEED();
	element = create_element( (struct pico_mqtt_packet*) MALLOC(sizeof(struct pico_mqtt_packet)) );
	ck_assert_msg(element != NULL, "Creating the element should not have failed\n");
	list->first = element;
	element->previous = NULL;

	element = create_element( (struct pico_mqtt_packet*) MALLOC(sizeof(struct pico_mqtt_packet)) );
	ck_assert_msg(element != NULL, "Creating the element should not have failed\n");
	list->first->next = element;
	element->previous = list->first;

	element = create_element( (struct pico_mqtt_packet*) MALLOC(sizeof(struct pico_mqtt_packet)) );
	ck_assert_msg(element != NULL, "Creating the element should not have failed\n");
	list->first->next->next = element;
	element->previous = list->first->next;
	element->next = NULL;
	list->last = element;

	list->length = 3;

	pico_mqtt_list_destroy(list);

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(get_element_by_index_test)
{
	int error = 0;
	struct pico_mqtt_list list = PICO_MQTT_LIST_EMPTY;
	uint32_t index = 0;
	uint32_t length = 10;
	struct element elements[10] = {
		ELEMENT_EMPTY,
		ELEMENT_EMPTY,
		ELEMENT_EMPTY,
		ELEMENT_EMPTY,
		ELEMENT_EMPTY,
		ELEMENT_EMPTY,
		ELEMENT_EMPTY,
		ELEMENT_EMPTY,
		ELEMENT_EMPTY,
		ELEMENT_EMPTY
	};

	list = (struct pico_mqtt_list){.length = 10, .first = &elements[0], .last = &elements[9], .error = &error};

	elements[0] = (struct element){.packet = (struct pico_mqtt_packet*) 0, .previous = NULL, .next = &elements[1]};
	elements[1] = (struct element){.packet = (struct pico_mqtt_packet*) 1, .previous = &elements[0], .next = &elements[2]};
	elements[2] = (struct element){.packet = (struct pico_mqtt_packet*) 2, .previous = &elements[1], .next = &elements[3]};
	elements[3] = (struct element){.packet = (struct pico_mqtt_packet*) 3, .previous = &elements[2], .next = &elements[4]};
	elements[4] = (struct element){.packet = (struct pico_mqtt_packet*) 4, .previous = &elements[3], .next = &elements[5]};
	elements[5] = (struct element){.packet = (struct pico_mqtt_packet*) 5, .previous = &elements[4], .next = &elements[6]};
	elements[6] = (struct element){.packet = (struct pico_mqtt_packet*) 6, .previous = &elements[5], .next = &elements[7]};
	elements[7] = (struct element){.packet = (struct pico_mqtt_packet*) 7, .previous = &elements[6], .next = &elements[8]};
	elements[8] = (struct element){.packet = (struct pico_mqtt_packet*) 8, .previous = &elements[7], .next = &elements[9]};
	elements[9] = (struct element){.packet = (struct pico_mqtt_packet*) 9, .previous = &elements[8], .next = NULL};

	for(length = 1; length <= 10; length++)
	{
		list.length = length;
		elements[length-1].next = NULL;
		if(length > 1)
			elements[length-2].next = &elements[length - 1];

		list.last = &elements[length - 1];

		for(index = 0; index < length; index++)
		{
			struct element* element = get_element_by_index(&list, index);
			ck_assert_msg(element != NULL, "An element should be found.\n");
			ck_assert_msg( element->packet == elements[index].packet, "The element returned is not the correct one.\n");
		}
	}

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(list_add_test)
{
	int error = 0;
	int result = 0;
	struct pico_mqtt_list* list = NULL;
	uint32_t index = 0;
	uint32_t length = 10;

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed.\n");

	{
		uint32_t* reference = (uint32_t*) MALLOC(sizeof(uint32_t));
		*reference = 0;
		MALLOC_FAIL_ONCE();
		PERROR_DISABLE_ONCE();
		result = list_add(list, (struct pico_mqtt_packet*) reference, 0);
		ck_assert_msg(result == ERROR, "Adding the element should have failed");
		FREE(reference);
	}

	pico_mqtt_list_destroy(list);

	CHECK_NO_ALLOCATIONS();

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed.\n");

	for(length = 1; length <= 10; length++)
	{
		uint32_t* reference = (uint32_t*) MALLOC(sizeof(uint32_t));
		*reference = length;
		result = list_add(list, (struct pico_mqtt_packet*) reference, length - 1);
		ck_assert_msg(result == SUCCES, "Adding the element should not have failed");
		ck_assert_msg(list->length == length, "The length property wasn't set correctly.");

		for(index = 0; index < length; index++)
		{
			struct element* element = get_element_by_index(list, index);
			ck_assert_msg(element != NULL, "An element should be found.\n");
			ck_assert_msg( *((uint32_t*)element->packet) == index + 1, "The element returned is not the correct one.\n");
		}
	}

	pico_mqtt_list_destroy(list);
	CHECK_NO_ALLOCATIONS();

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed.\n");

	for(length = 1; length <= 10; length++)
	{
		uint32_t* reference = (uint32_t*) MALLOC(sizeof(uint32_t));
		*reference = length;
		result = list_add(list, (struct pico_mqtt_packet*) reference, 0);
		ck_assert_msg(result == SUCCES, "Adding the element should not have failed");
		ck_assert_msg(list->length == length, "The length property wasn't set correctly.");


		for(index = 0; index < length; index++)
		{
			struct element* element = get_element_by_index(list, index);
			ck_assert_msg(element != NULL, "An element should be found.\n");
			ck_assert_msg( *((uint32_t*)element->packet) == length - index, "The element returned is not the correct one.\n");
		}
	}

	pico_mqtt_list_destroy(list);
	CHECK_NO_ALLOCATIONS();

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed.\n");

	for(length = 1; length <= 10; length++)
	{
		uint32_t* reference = (uint32_t*) MALLOC(sizeof(uint32_t));
		*reference = length;
		result = list_add(list, (struct pico_mqtt_packet*) reference, length - 1);
		ck_assert_msg(result == SUCCES, "Adding the element should not have failed");
		ck_assert_msg(list->length == length, "The length property wasn't set correctly.");

		for(index = 0; index < length; index++)
		{
			struct element* element = get_element_by_index(list, index);
			ck_assert_msg(element != NULL, "An element should be found.\n");
			ck_assert_msg( *((uint32_t*)element->packet) == index + 1, "The element returned is not the correct one.\n");
		}
	}

	{
		uint32_t* reference = (uint32_t*) MALLOC(sizeof(uint32_t));
		*reference = 101;
		result = list_add(list, (struct pico_mqtt_packet*) reference, 5);
	}

	for(index = 0; index < 5; index++)
	{
		struct element* element = get_element_by_index(list, index);
		ck_assert_msg(element != NULL, "An element should be found.\n");
		ck_assert_msg( *((uint32_t*)element->packet) == index + 1, "The element returned is not the correct one.\n");
	}

	{
		struct element* element = get_element_by_index(list, 5);
		ck_assert_msg(element != NULL, "An element should be found.\n");
		ck_assert_msg( *((uint32_t*)element->packet) == 101, "The element returned is not the correct one.\n");
	}

	for(index = 6; index < length; index++)
	{
		struct element* element = get_element_by_index(list, index);
		ck_assert_msg(element != NULL, "An element should be found.\n");
		ck_assert_msg( *((uint32_t*)element->packet) == index, "The element returned is not the correct one.\n");
	}

	pico_mqtt_list_destroy(list);
	
	CHECK_NO_ALLOCATIONS();
}
END_TEST


START_TEST(remove_element_test)
{
	int error = 0;
	struct pico_mqtt_list* list = NULL;
	uint32_t index = 0;
	int result = 0;

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed");

	for(index = 0; index < 4; index++)
	{
		uint32_t* reference = (uint32_t*) MALLOC(sizeof(uint32_t));
		*reference = index;
		result = list_add(list, (struct pico_mqtt_packet*) reference, index);
		ck_assert_msg(result == SUCCES, "Adding the element should not have failed");
	}

	FREE(get_element_by_index(list, 1)->packet);
	remove_element(list, get_element_by_index(list, 1));
	ck_assert_msg(list->first->next == list->last->previous, "Removing the element did not set the correct pointers.\n");
	ck_assert_msg(*((uint32_t*)(get_element_by_index(list, 0)->packet)) == 0, "Unexpected element.\n");
	ck_assert_msg(*((uint32_t*)(get_element_by_index(list, 1)->packet)) == 2, "Unexpected element.\n");
	ck_assert_msg(*((uint32_t*)(get_element_by_index(list, 2)->packet)) == 3, "Unexpected element.\n");

	FREE(get_element_by_index(list, 0)->packet);
	remove_element(list, get_element_by_index(list, 0));
	ck_assert_msg(list->first->next == list->last, "Removing the element did not set the correct pointers.\n");
	ck_assert_msg(list->first == list->last->previous, "Removing the element did not set the correct pointers.\n");
	ck_assert_msg(*((uint32_t*)(get_element_by_index(list, 0)->packet)) == 2, "Unexpected element.\n");
	ck_assert_msg(*((uint32_t*)(get_element_by_index(list, 1)->packet)) == 3, "Unexpected element.\n");

	FREE(get_element_by_index(list, 1)->packet);
	remove_element(list, get_element_by_index(list, 1));
	ck_assert_msg(list->first == list->last, "Removing the element did not set the correct pointers.\n");
	ck_assert_msg(*((uint32_t*)(get_element_by_index(list, 0)->packet)) == 2, "Unexpected element.\n");

	FREE(get_element_by_index(list, 0)->packet);
	remove_element(list, get_element_by_index(list, 0));
	ck_assert_msg(list->first == NULL, "Removing the element did not set the correct pointers.\n");
	ck_assert_msg(list->last == NULL, "Removing the element did not set the correct pointers.\n");

	pico_mqtt_list_destroy(list);

	CHECK_NO_ALLOCATIONS();
}
END_TEST


START_TEST(pico_mqtt_list_push_back_test)
{
	int error = 0;
	struct pico_mqtt_list* list = NULL;
	uint32_t* reference = NULL;
	uint32_t index = 0;
	int result = 0;

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed");

	for(index = 0; index < 10; ++index)
	{
		reference = (uint32_t*) MALLOC(sizeof(uint32_t));
		*reference = index;
		result = pico_mqtt_list_push_back(list, (struct pico_mqtt_packet*) reference);
		ck_assert_msg(result == SUCCES, "Adding the element to the list failed.\n");
	}

	ck_assert_msg(list->length == 10, "The list has an Unexpected length.\n");

	for(index = 0; index < 10; ++index)
	{
		ck_assert_msg(*((uint32_t*)(get_element_by_index(list, index)->packet)) == index, "Unexpected element.\n");
	}

	pico_mqtt_list_destroy(list);

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_list_pop_test)
{
	int error = 0;
	struct pico_mqtt_list* list = NULL;
	uint32_t* reference = NULL;
	uint32_t index = 0;
	int result = 0;

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed");

	for(index = 0; index < 10; ++index)
	{
		reference = (uint32_t*) MALLOC(sizeof(uint32_t));
		*reference = index;
		result = pico_mqtt_list_push_back(list, (struct pico_mqtt_packet*) reference);
		ck_assert_msg(result == SUCCES, "Adding the element to the list failed.\n");
	}

	ck_assert_msg(list->length == 10, "The list has an Unexpected length.\n");

	for(index = 0; index < 10; ++index)
	{
		struct pico_mqtt_packet* packet = pico_mqtt_list_pop(list);
		ck_assert_msg(*((uint32_t*) packet ) == index, "Unexpected element.\n");
		ck_assert_msg(list->length == 9-index, "The list doesn't have the correct length.\n");
		FREE(packet);

		if(index == 9)
		{
			ck_assert_msg( list->first == NULL, "The pointers are not set correctly.\n");
			ck_assert_msg( list->last == NULL, "The pointers are not set correctly.\n");
		} else 
		{
			struct element* element = get_element_by_index(list, 0);
			ck_assert_msg( *((uint32_t*)(element->packet)) == index + 1, "The first element is not set correctly.\n");
			ck_assert_msg( list->first == element, "The list pointers are not set correctly.\n");
		}
	}

	{
		struct pico_mqtt_packet* packet = NULL;
		CHECK_DISABLE();
		packet = pico_mqtt_list_pop(list);
		ck_assert_msg(packet == NULL, "popping an element from an empty list should return NULL.\n");
		CHECK_ENABLE();
	}
	pico_mqtt_list_destroy(list);

	CHECK_NO_ALLOCATIONS();
}
END_TEST

START_TEST(pico_mqtt_list_get_test)
{
	int error = 0;
	struct pico_mqtt_list* list = NULL;
	struct pico_mqtt_packet* packet = NULL;
	uint16_t index = 0;
	int result = 0;

	list = pico_mqtt_list_create( &error );
	ck_assert_msg(list != NULL, "Creating the list should not have failed");

	for(index = 0; index < 10; ++index)
	{
		packet = (struct pico_mqtt_packet*) MALLOC(sizeof(packet));
		packet->packet_id = index;
		result = pico_mqtt_list_push_back(list, packet);
		ck_assert_msg(result == SUCCES, "Adding the element to the list failed.\n");
	}

	ck_assert_msg(list->length == 10, "The list has an Unexpected length.\n");

	packet = pico_mqtt_list_get(list, 100);
	ck_assert_msg(packet == NULL, "getting an unexisting element from a list should return NULL.\n");

	for(index = 0; index < 10; ++index)
	{
		packet = pico_mqtt_list_get(list, index);
		ck_assert_msg( packet->packet_id == index, "Unexpected element.\n");
		ck_assert_msg(list->length == (uint16_t) (9 - index), "The list doesn't have the correct length.\n");
		FREE(packet);

		if(index == 9)
		{
			ck_assert_msg( list->first == NULL, "The pointers are not set correctly.\n");
			ck_assert_msg( list->last == NULL, "The pointers are not set correctly.\n");
		} else 
		{
			struct element* element = get_element_by_index(list, 0);
			ck_assert_msg( element->packet->packet_id == index + 1, "The first element is not set correctly.\n");
			ck_assert_msg( list->first == element, "The list pointers are not set correctly.\n");
		}
	}

	packet = pico_mqtt_list_get(list, 100);
	ck_assert_msg(packet == NULL, "Getting an element from an empty list should return NULL.\n");

	pico_mqtt_list_destroy(list);

	CHECK_NO_ALLOCATIONS();
}
END_TEST

Suite * functional_list_suite(void)
{
	Suite *test_suite;
	TCase *test_case_core;

	test_suite = suite_create("MQTT serializer");

	/* Core test case */
	test_case_core = tcase_create("Core");

	tcase_add_test(test_case_core, compare_arrays_test);
	tcase_add_test(test_case_core, debug_malloc_test);
	tcase_add_test(test_case_core, pico_mqtt_list_create_test);
	tcase_add_test(test_case_core, create_element_test);
	tcase_add_test(test_case_core, pico_mqtt_list_length_test);
	tcase_add_test(test_case_core, destroy_element_test);
	tcase_add_test(test_case_core, get_element_by_index_test);
	tcase_add_test(test_case_core, pico_mttq_list_destroy_test);
	tcase_add_test(test_case_core, list_add_test);
	tcase_add_test(test_case_core, remove_element_test);
	tcase_add_test(test_case_core, pico_mqtt_list_push_back_test);
	tcase_add_test(test_case_core, pico_mqtt_list_pop_test);
	tcase_add_test(test_case_core, pico_mqtt_list_get_test);

	suite_add_tcase(test_suite, test_case_core);

	return test_suite;
}

 int main(void)
 {
	int number_failed;
	Suite *test_suite;
	SRunner *suite_runner;

	test_suite = functional_list_suite();
	suite_runner = srunner_create(test_suite);

	srunner_run_all(suite_runner, CK_NORMAL);
	number_failed = srunner_ntests_failed(suite_runner);
	srunner_free(suite_runner);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
 }

/**
* test functions implementation
**/


int compare_arrays(void* a, void* b, uint32_t length)
{
	uint32_t index = 0;
	for (index = 0; index < length; ++index)
	{
		/*printf("--- %02X - %02X\n", *((uint8_t*)a + index), *((uint8_t*)b + index));*/
		if(*((uint8_t*)a + index) != *((uint8_t*)b + index))
			return 0;
	}

	return 1;
}
/*
int check_list_integrity(struct pico_mqtt_list* list)
{
	int error_flag = 0;
	int count = 0;
	struct pico_mqtt_element* current = NULL;

	if(list->length == 0)
	{
		if(list->first == NULL || list->last == NULL)
			return SUCCES;
	}

	current = list->first;
	count++;

	if(current == NULL || current->previous != NULL)
		return ERROR;

	if(current->next == NULL)
	{
		if(count == list->length && list->last == current)
		{
			return SUCCES;
		} else {
			return ERROR;
		}
	}
}*/