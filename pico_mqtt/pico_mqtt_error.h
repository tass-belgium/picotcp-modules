#ifndef PICO_MQTT_ERROR_H
#define PICO_MQTT_ERROR_H

/**
* Error codes
**/

#define NO_ERROR 0
#define TIMEOUT 1
#define CONNECTION_BLOCKING 2
#define URI_LOOKUP_FAILED 3

/**
* Return codes
**/

#define SUCCES 0
#define ERROR -1

/**
* Debug Levels
**

0 or not defined : no error logging
1: print all errors
2: print all warnings
3: print all info
4: print all todo's

*/

#define PERROR(args...) do{} while(0)
#define PWARNING(args...) do{} while(0)
#define PINFO(args...) do{} while(0)
#define PTODO(args...) do{} while(0)
#define EXIT_ERROR() do{} while(0)
#define PTRACE() do{} while(0)
#define MALLOC(size)  malloc(size)
#define FREE(pointer) free(pointer)
#define CHECK_DISABLE()
#define CHECK_ENABLE()
#define CHECK_DISABLE_ONCE()
#define PERROR_DISABLE()
#define PERROR_ENABLE()
#define PERROR_DISABLE_ONCE()
#define MALLOC_FAIL()
#define MALLOC_SUCCEED()
#define MALLOC_FAIL_ONCE()
#define CHECK( cond, ...) do{} while(0)
#define CHECK_NULL( var ) do{} while(0)
#define CHECK_NOT_NULL( var ) do{} while(0)
#define CHECK_ALLOCATIONS( var ) do{} while(0)
#define CHECK_NO_ALLOCATIONS() CHECK_ALLOCATIONS(0)

#ifndef PRODUCTION

	#ifdef DEBUG /* if not in debug mode, nothing should be printed */

		struct debug{
			uint8_t no_check_flag;
			uint8_t auto_reset_no_check_flag;
			uint8_t no_error_flag;
			uint8_t auto_reset_no_error_flag;
			uint8_t fail_flag;
			uint8_t auto_reset_fail_flag;
			uint32_t allocations;
			uint32_t frees;
			uint32_t total_allocated;
			uint32_t max_bytes_allocated;

		};

		static struct debug debug = (struct debug){
			.no_check_flag = 0,
			.auto_reset_no_check_flag = 0,
			.no_error_flag = 0,
			.auto_reset_no_error_flag = 0,
			.fail_flag = 0,
			.auto_reset_fail_flag = 0,
			.allocations = 0,
			.frees = 0,
			.total_allocated = 0,
			.max_bytes_allocated = 0
		};

		#if DEBUG > 0
			#ifndef PEDANTIC
				#define PEDANTIC
			#endif
			
			#ifndef ENABLE_ERROR
				#define ENABLE_ERROR
			#endif

			#ifndef ENABLE_TRACE
				#ifndef DISABLE_TRACE
					#define ENABLE_TRACE
				#endif
			#endif

		#endif

		#if DEBUG > 1
			#ifndef ENABLE_WARNING
				#define ENABLE_WARNING
			#endif
		#endif

		#if DEBUG > 2
			#ifndef ENABLE_INFO
				#define ENABLE_INFO
			#endif
		#endif

		#if DEBUG > 3
			#ifndef ENABLE_TODO
				#define ENABLE_TODO
			#endif
		#endif

		#ifndef DISABLE_EXIT
			#include <stdlib.h>

			#undef EXIT_ERROR
			#define EXIT_ERROR() exit(EXIT_FAILURE)
		#endif
			

		#ifdef ENABLE_ERROR
			#include <stdio.h>

			#undef PERROR
			#undef PERROR_DISABLE
			#undef PERROR_ENABLE
			#undef PERROR_DISABLE_ONCE

			#define PERROR_DISABLE() debug.no_error_flag = 1; debug.auto_reset_no_error_flag = 0
			#define PERROR_ENABLE() debug.no_error_flag = 0; debug.auto_reset_no_error_flag = 0
			#define PERROR_DISABLE_ONCE() debug.no_error_flag = 1; debug.auto_reset_no_error_flag = 1
			#define PERROR( ...) do{\
					if(debug.no_error_flag == 0){\
						printf("[ERROR] in %s - %s at line %d:\t\t", __FILE__,__func__,__LINE__);\
						printf(__VA_ARGS__);\
					}else {\
						if(debug.auto_reset_no_error_flag != 0)\
							debug.no_error_flag = 0;\
							debug.auto_reset_no_error_flag = 0;\
					}\
				} while(0)
		#endif /* DEBUG >= 1 */

		#ifdef ENABLE_WARNING
			#include <stdio.h>

			#undef PWARNING
			#define PWARNING(...) do{\
				printf("[WARNING] in %s - %s at line %d:\t\t", __FILE__,__func__,__LINE__);\
				printf(__VA_ARGS__);\
				} while(0)
		#endif /* DEBUG >= 2 */

		#ifdef ENABLE_INFO
			#include <stdio.h>

			#undef PINFO
			#define PINFO(...) do{\
				printf("[INFO] in %s - %s at line %d:\t\t", __FILE__,__func__,__LINE__);\
				printf(__VA_ARGS__);\
				} while(0)
		#endif /* DEBUG >= 3*/

		#ifdef ENABLE_TODO
			#include <stdio.h>
		
			#undef PTODO
			#define PTODO(...) do{\
				printf("[TODO] in %s - %s at line %d:\t\t", __FILE__,__func__,__LINE__);\
				printf(__VA_ARGS__);\
				} while(0)
		#endif /*defined TODO */

		#ifdef ENABLE_TRACE
			#include <stdio.h>

			#undef PTRACE
			#define PTRACE() do{\
				printf("[TRACE] in %s - %s at line %d: called function returned error.\n", __FILE__,__func__,__LINE__);\
				} while(0)
		#endif


	#endif /* defined DEBUG */

	#ifdef PEDANTIC

		#undef CHECK_DISABLE
		#undef CHECK_ENABLE
		#undef CHECK_DISABLE_ONCE
		#undef CHECK
		#undef CHECK_NOT_NULL
		#undef CHECK_NULL

		#define CHECK_DISABLE() debug.no_check_flag = 1;\
								debug.auto_reset_no_check_flag = 0
		#define CHECK_ENABLE() debug.no_check_flag = 0;\
								debug.auto_reset_no_check_flag = 0
		#define CHECK_DISABLE_ONCE() debug.no_check_flag = 1;\
									 debug.auto_reset_no_check_flag = 1
		#define CHECK( cond, ...) do{\
			if(debug.no_check_flag != 0){\
				if(debug.auto_reset_no_check_flag != 0){\
					debug.auto_reset_no_check_flag = 0;\
					debug.no_check_flag = 0;}} else {\
				if(!(cond)){PERROR(__VA_ARGS__); EXIT_ERROR();}}} while(0)

		#define CHECK_NOT_NULL( var ) CHECK( var != NULL, "The pointer should not be NULL.\n")
		#define CHECK_NULL( var ) CHECK( var == NULL, "The pointer should be NULL.\n")
		
	#endif /* ifndef PEDANTIC */

	#ifndef DEBUG_MALLOC
		#define DEBUG_MALLOC
		#include <stdio.h>
		#undef MALLOC
		#undef MALLOC_FAIL
		#undef MALLOC_SUCCEED
		#undef MALLOC_FAIL_ONCE
		#undef FREE
		#undef CHECK_ALLOCATIONS

			static void* my_debug_malloc(size_t length)
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

			static void my_debug_free(void* pointer)
			{
				debug.frees++;
				free(pointer);
			}

		#define MALLOC(size) my_debug_malloc(size)
		#define MALLOC_FAIL() debug.fail_flag = 1; debug.auto_reset_fail_flag = 0
		#define MALLOC_SUCCEED() debug.fail_flag = 0; debug.auto_reset_fail_flag = 0
		#define MALLOC_FAIL_ONCE() debug.fail_flag = 1; debug.auto_reset_fail_flag = 1
		#define FREE(pointer) my_debug_free(pointer)
		#define CHECK_ALLOCATIONS(var) do{\
			int32_t allocated = (int32_t)debug.allocations - (int32_t)debug.frees;\
			if(allocated < 0){\
				printf("[LEAK] in %s - %s at line %d:\t\t", __FILE__,__func__,__LINE__);\
				printf("Their are more frees than allocations.\n");EXIT_ERROR();}\
			if(allocated != var){\
				printf("[LEAK] in %s - %s at line %d:\t\t", __FILE__,__func__,__LINE__);\
				printf("There are more chunks allocated then expected (%d instead of %d).\n", debug.allocations - debug.frees, var); EXIT_ERROR();}}while(0)
		#define ALLOCATION_REPORT() do{\
			printf("\nMemory allocation report:\n");\
			printf("Total memory allocated: %ld\n", (long int) debug.total_allocated);\
			printf("Allocations: %ld Frees: %ld\n", (long int) debug.allocations, (long int) debug.frees);\
			printf("Biggest allocations: %ld\n", (long int) debug.max_bytes_allocated);\
			printf("Currently allocated: %ld\n\n", (long int) debug.allocations - (long int) debug.frees);}while(0)
	#endif /* ifndef DEBUG_MALLOC */

	/**
	* Adds extra checks for errors during development,
	* does not change code.
	**/

#endif /* ifndef PRODUCTION */

#endif /* end of header file */
