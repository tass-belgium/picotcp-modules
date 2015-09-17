#ifndef PICO_MQTT_ERROR_H
#define PICO_MQTT_ERROR_H

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

#ifdef DEBUG
	#if DEBUG >= 1
		#include <stdio.h> /* needed for printf */
		#include <string.h> /* needed for strerror */

		#undef PERROR	
		#define PERROR(...) do{\
			printf("[ERROR] in %s at line %d:\t\t", __FILE__, __LINE__);\
			printf(__VA_ARGS__);\
			} while(0)
	#endif /* DEBUG >= 1 */

	#if DEBUG >= 2
		#undef PWARNING
		#define PWARNING(...) do{\
			printf("[WARNING] in %s at line %d:\t\t", __FILE__, __LINE__);\
			printf(__VA_ARGS__);\
			} while(0)
	#endif /* DEBUG >= 2 */

	#if DEBUG >= 3
		#undef PINFO
		#define PINFO(...) do{\
			printf("[INFO] in %s at line %d:\t\t", __FILE__, __LINE__);\
			printf(__VA_ARGS__);\
			} while(0)
	#endif /* DEBUG >= 3*/
#endif /* defined DEBUG */

#ifdef TODO
	#if TODO == 1
		#include <stdio.h>
	
		#define PTODO(...) do{\
			printf("[TODO] in %s at line %d:\t\t", __FILE__, __LINE__);\
			printf(__VA_ARGS__);\
			} while(0)
	#endif /* TODO >= 1 */

	#if TODO >= 2
		#include <stdio.h>
		
		#define PTODO(...) do{\
			printf("[TODO] in %s at line %d:\t\t", __FILE__, __LINE__);\
			printf(__VA_ARGS__);\
			#warning "Some tasks are not yet completed"\
			} while(0)
	#endif /* TODO >= 2 */
#else
	#define PTODO(args...) do{} while(0)
#endif /*defined TODO */

#define RETURN_IF_ERROR(x) do{if(x==ERROR){return ERROR;}} while(0)

#endif
