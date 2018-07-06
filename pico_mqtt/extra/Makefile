CC      = $(TCPREFIX)gcc
AS      = $(TCPREFIX)gcc
LD      = $(TCPREFIX)gcc
CP      = $(TCPREFIX)objcopy
OD      = $(TCPREFIX)objdump

CORE_FILES := obj/pico_mqtt.o
CORE_FILES += obj/pico_mqtt_list.o
CORE_FILES += obj/pico_mqtt_serializer.o
CORE_FILES += obj/pico_mqtt_socket.o
CORE_FILES += obj/pico_mqtt_stream.o
CORE_FILES += obj/pico_mqtt_debug.o
ASM_FILES := $(wildcard ../*.s)
CXX_FILES := $(wildcard ../*.c)
OBJ_FILES := $(addprefix obj/,$(notdir $(CXX_FILES:.c=.o)))
OBJ_FILES += $(addprefix obj/,$(notdir $(ASM_FILES:.s=.o)))
INC_DIRS := ../
INC_DIRS := $(addprefix -I,$(INC_DIRS))
OUT_FILE := out/main
BUILD_DIR := build

CFLAGS  =  -I. $(INC_DIRS) -g -O0 -Wall
#CFLAGS += -fdiagnostics-color

LFLAGS  = $(CFLAGS)
CPFLAGS = -Obinary
ODFLAGS = -S




################################################################
## Colordefinition
################################################################
NO_COLOR        = #\x1b[0m
GREEN_COLOR     = #\x1b[32;01m
YELLOW_COLOR    = #\x1b[33;01m
RED_COLOR       = #\x1b[31;01m
WHITE_COLOR     = #\x1b[37;01m

all: clean demo
	

demo: demo.elf
	@echo 'Created gardenDemo.elf'

# Test TCP/IP connection with "make server" and "make client"
server: tcp_server simple_tcp_server_ipv6
	@echo 'Created tcp_server and simple_tcp_server_ipv6'

client: tcp_client simple_tcp_client_ipv6
	@echo 'Created tcp_client and simple_tcp_client_ipv6'

# Helpers
init: 
	mkdir -p out obj 

clean:
	@echo -e '$(WHITE_COLOR)>>> CLEANING$(NO_COLOR)'
	-rm -f main.lst obj/*.o out/*.elf out/*.lst out/*.bin *.o *.elf simple_tcp_server_ipv6 simple_tcp_client_ipv6 tcp_server tcp_client


################################################################
## binary files (NOT USED)
################################################################
$(OUT_FILE).bin: $(OUT_FILE).elf
	@echo -e '$(WHITE_COLOR)>>> COPYING$(NO_COLOR)'
	$(CP) $(CPFLAGS) $(OUT_FILE).elf $(OUT_FILE).bin
	$(CP) $(CPFLAGS) $(OUT_FILE).elf -O srec --srec-len 19 $(OUT_FILE).s19
	$(CP) $(CPFLAGS) $(OUT_FILE).elf -O ihex $(OUT_FILE).hex
#the .lst is an assembly listing - just here for educational purposes
	$(OD) $(ODFLAGS) $(OUT_FILE).elf> $(OUT_FILE).lst

################################################################
## unspecified .elf files
################################################################
$(OUT_FILE).elf: $(OBJ_FILES)
	@echo -e '$(WHITE_COLOR)>>> LINKING$(NO_COLOR)'
	$(LD) $(LFLAGS) -o $@ $^ -Wl,-Map,$(OUT_FILE).map
	
demo.elf: $(CORE_FILES) obj/garden_watering_example.o demo.o
	@echo -e '$(WHITE_COLOR)>>> LINKING$(NO_COLOR)'
	$(LD) $(LFLAGS) $^ -o $@

.o: %.c
	@echo -e '$(WHITE_COLOR)>>> COMPILING [C] $^$(NO_COLOR)'
	$(CC) -c $(CFLAGS) $< -o $@


obj/%.o: ../%.c
	@echo -e '$(WHITE_COLOR)>>> COMPILING [C] $^$(NO_COLOR)'
	$(CC) -c $(CFLAGS) $< -o $@


