#include "pico_mqtt_debug.h"

/**
* The global debug object containing all debug info of all files
**/

struct debug debug = DEBUG_EMPTY;

/**
* private function prototypes
**/

static void add_allocation( void* memory_pointer, uint64_t size, const char* file, const char* func, uint32_t line);
static uint64_t remove_allocation( void* memory_pointer, const char* file, const char* func, uint32_t line);

/**
* public function implementation
**/

void* my_debug_malloc(size_t length, const char* file, const char* func, uint32_t line)
{
	void* return_value = NULL;

	if(length == 0)
		printf("[MEMORY] Attempting to allocate 0 byte at: %s - %s at line %d \n", file, func, line);

	if(debug.fail_flag == 0)
	{
		return_value = malloc(length);

		if(return_value != NULL)
		{
			add_allocation( return_value, length, file, func, line);

			debug.allocations++;
			debug.total_allocated += (uint32_t) length;
			debug.currently_allocated += length;

			if(length > debug.max_bytes_allocated)
				debug.max_bytes_allocated = (uint32_t) length;

			return return_value;
		} else
		{
			PERROR("System is out of memory, infinit loop with allocation?\n");
			exit(12);
		}

		return NULL;
	}

	if(debug.auto_reset_fail_flag == 1)
	{
		debug.fail_flag = 0;
		debug.auto_reset_fail_flag = 0;
	}

	return NULL;
}

void my_debug_free( void* pointer, const char* file, const char* func, uint32_t line )
{
	if(pointer == NULL)
		return;

	debug.currently_allocated -= remove_allocation( pointer, file, func, line);

	debug.frees++;
	free(pointer);
}

/**
* private function implementation
**/

static void add_allocation( void* memory_pointer, uint64_t size, const char* file, const char* func, uint32_t line)
{
	struct allocation* allocation = (struct allocation*) malloc(sizeof(struct allocation));
	struct allocation* current = debug.allocation_list;
	struct allocation* previous = debug.allocation_list;

	if(allocation == NULL)
	{
		PERROR("System is out of memory, infinit loop with allocation?\n");
		exit(1);
	}

	allocation->size = size;
	allocation->memory_pointer = memory_pointer;
	allocation->file = file;
	allocation->func = func;
	allocation->line = line;
	allocation->next = NULL;

	if(debug.allocation_list == NULL)
	{
		debug.allocation_list = allocation;
		return;
	}

	while(current != NULL)
	{
		previous = current;
		current = current->next;
	}
	
	previous->next = allocation;
}

static uint64_t remove_allocation( void* memory_pointer, const char* file, const char* func, uint32_t line)
{
	struct allocation* current = debug.allocation_list;
	struct allocation** next_ptr = &(debug.allocation_list);
	uint64_t max_allocation = debug.allocations + 2;

	if(debug.allocation_list == NULL)
	{
		printf("[MEMORY] Attempting to free an untracked memory fragment, make sure to use MALLOC macro for allocations.\n");
		exit(1);
	}

	while((current != NULL) && (max_allocation != 0))
	{
		--max_allocation;

		if(current->memory_pointer == memory_pointer)
		{
			uint64_t size = current->size;
			*next_ptr = current->next;
			free(current);
			return size;
		} else {
			next_ptr = &(current->next);
			current = current->next;
		}
	}

	printf("[MEMORY] Attempting to free an untracked memory fragment at: %s - %s at line %d \n", file, func, line);
	exit(1);
}

void print_allocation_table( void )
{


	if(debug.allocation_list == NULL)
	{
		printf("+--------------------------------------+\n");
		printf("|            NO ALLOCATIONS            |\n");
		if(debug.unit_test != NULL)
			printf("| test: %-30s |\n", debug.unit_test);
		printf("+--------------------------------------+\n");
	} else {
		struct allocation* current = debug.allocation_list;

		printf("+--------------------------------------------------------------------------------------------------+\n");
		printf("|                                             ALLOCATION TABLE                                     |\n");
		if(debug.unit_test != NULL)
			printf("|                                    test: %-30s                          |\n", debug.unit_test);
		printf("+-----------------+----------------------------------+--------------------------------------+------+\n");
		printf("| BYTES ALLOCATED |               FILE               |               FUNCTION               | LINE |\n");
		printf("+-----------------+----------------------------------+--------------------------------------+------+\n");


		while(current != NULL)
		{

			printf("| %-15ld |  %-30s  | %-36s | %-4d |\n", current->size, current->file, current->func, current->line);
			printf("+-----------------+----------------------------------+--------------------------------------+------+\n");
			current = current->next;
		}
		printf("\n");
	}
}

#ifdef DEBUG

void print_unit_test( void )
{
	if(debug.printed_unit_test != 0)
		return;

	debug.printed_unit_test = 1;

	printf("\n+--------------------------------------+\n");
	if(debug.unit_test != NULL)
	{
		printf("| test: %-30s |\n", debug.unit_test);
		printf("+--------------------------------------+\n");
	}
}

#endif