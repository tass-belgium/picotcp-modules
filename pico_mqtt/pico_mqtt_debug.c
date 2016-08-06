#include "pico_mqtt_debug.h"

struct debug debug = DEBUG_EMPTY;

void* my_debug_malloc(size_t length)
{
	void* return_value = NULL;

	CHECK(length != 0, "Attempting to allocate 0 byte.\n");

	if(debug.fail_flag == 0)
	{
		return_value = malloc(length);

		if(return_value != NULL)
		{
			debug.allocations++;
			debug.total_allocated += (uint32_t) length;

			if(length > debug.max_bytes_allocated)
				debug.max_bytes_allocated = (uint32_t) length;
		}

		return return_value;
	}

	if(debug.auto_reset_fail_flag == 1)
	{
		debug.fail_flag = 0;
		debug.auto_reset_fail_flag = 0;
	}

	return NULL;
}

void my_debug_free(void* pointer)
{
	if(pointer == NULL)
		return;

	debug.frees++;
	free(pointer);
}