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

*/

/**
* TODO Levels
**

0 or not defined : no TODO's displayed
1: print all todos in code you use
2: print warnings for all TODO's
3: print errors for all TODO's

*/

#define PERROR(args...) do{} while(0)
#define PWARNING(args...) do{} while(0)
#define PINFO(args...) do{} while(0)
#define PTODO(args...) do{} while(0)
#define EXIT_ERROR() do{} while(0)
#define PTRACE do{} while(0)

#ifdef DEBUG /* if not in debug mode, nothing should be printed */

	#if DEBUG > 0
		#ifndef ENABLE_ERROR
			#define ENABLE_ERROR
		#endif

		#ifndef ENABLE_TRACE
			#define ENABLE_TRACE
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

	#ifndef DISABLE_EXIT
		#include <stdlib.h>

		#undef EXIT_ERROR
		#define EXIT_ERROR() exit(EXIT_FAILURE)
	#endif
		

	#ifdef ENABLE_ERROR
		#include <stdio.h>

		#undef PERROR	
		#define PERROR( ...) do{\
			printf("[ERROR] in %s - %s at line %d:\t\t", __FILE__,__func__,__LINE__);\
			printf(__VA_ARGS__);\
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
		#define PTRACE do{\
			printf("[TRACE] in %s - %s at line %d: called function returned error.\n", __FILE__,__func__,__LINE__);\
			} while(0)
	#endif


#endif /* defined DEBUG */

/**
* Adds extra checks for errors during development,
* does not change code.
**/

#ifndef PEDANTIC

	#define PICO_MQTT_CHECK( cond, ...) do{} while(0)
	#define PICO_MQTT_CHECK_NULL( var ) do{} while(0)
	#define PICO_MQTT_CHECK_NOT_NULL( var ) do{} while(0)

#else

	#define PICO_MQTT_CHECK( cond, ...) do{\
		if(!(cond)){PERROR(__VA_ARGS__); EXIT_ERROR();}} while(0)

	#define PICO_MQTT_CHECK_NOT_NULL( var ) do{\
		if( var == NULL){PERROR("The pointer should not be NULL.\n"); EXIT_ERROR();}} while(0)

	#define PICO_MQTT_CHECK_NULL( var ) do{\
		if( var != NULL){PERROR("The pointer should be NULL.\n"); EXIT_ERROR();}} while(0)

#endif /* ifndef PEDANTIC */

#endif /* end of header file */
