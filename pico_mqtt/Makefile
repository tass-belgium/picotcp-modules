CC=gcc
DEBUG_FLAGS= -DDEBUG=3 -g 
C_FLAGS= -Wdeclaration-after-statement -W -Wshadow -Wcast-qual -Wwrite-strings -Wundef -Wconversion -Wcast-align -Wmissing-prototypes -Wno-missing-field-initializers -Wunused-function -Wextra -Werror -Wunused-variable -Wall
TEST_FLAGS= -lcheck -lpthread -lm -lrt -lsubunit
COVERAGE_FLAGS= -fprofile-arcs -ftest-coverage
COMPLEXITY_THRESHOLD= 3

OUTPUT_LOG= make_log.txt

#####################################################################################
# List of all files per category													#
#####################################################################################

SOURCE_FILES = $(wildcard *.c)

MOCK_OBJECT_FILES = $(addsuffix .o, $(basename $(wildcard *_mock.c)))
TEST_OBJECT_FILES = $(addsuffix .o, $(basename $(wildcard *_test.c)))
BASE_OBJECT_FILES = $(filter-out $(MOCK_OBJECT_FILES) $(TEST_OBJECT_FILES), $(addsuffix .o, $(basename $(wildcard *.c))))
ALL_OBJECT_FILES =  $(BASE_OBJECT_FILES) $(MOCK_OBJECT_FILES) $(TEST_OBJECT_FILES) 

UNIT_TESTS = $(addsuffix .out, $(basename $(TEST_OBJECT_FILES)))

#####################################################################################
# Main command for unit testing														#
#####################################################################################

unit_test: clear_log $(ALL_OBJECT_FILES) $(UNIT_TESTS) 

#####################################################################################
# Automatically generate dependencies												#
#####################################################################################

#http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) $(C_FLAGS) $(COVERAGE_FLAGS) $(DEBUG_FLAGS) -c
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

%.o : %.c
%.o : %.c $(DEPDIR)/%.d
		@$(COMPILE.c) $(OUTPUT_OPTION) $<
		@$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

-include $(patsubst %,$(DEPDIR)/%.d,$(basename $(SOURCE_FILES)))




#####################################################################################
# Run the unit test and print to file												#
#####################################################################################
clear_log:
	@rm -f $(OUTPUT_LOG)

%_test.out: %_test.o $(MOCK_OBJECT_FILES) pico_mqtt_debug.o
	$(CC) $(C_FLAGS) $(filter-out $(subst _test.out,_mock.o, $@), $^) -o $@ $(TEST_FLAGS) $(COVERAGE_FLAGS) $(DEBUG_FLAGS)
	@printf "+---------------------------------------------------------------+\n" 	| tee -a $(OUTPUT_LOG)
	@printf "| %-60s  |\n" "$@"														| tee -a $(OUTPUT_LOG)
	@printf "+---------------------------------------------------------------+\n"	| tee -a $(OUTPUT_LOG)
	@echo																			| tee -a $(OUTPUT_LOG)
	@./$@																			| tee -a $(OUTPUT_LOG)
	@echo																			| tee -a $(OUTPUT_LOG)
	@printf "+-----------------------------+\n"										| tee -a $(OUTPUT_LOG)
	@printf "| Complexity  threshold:%2d    |\n" $(COMPLEXITY_THRESHOLD)			| tee -a $(OUTPUT_LOG)
	@printf "+-----------------------------+\n"										| tee -a $(OUTPUT_LOG)
	@echo																			| tee -a $(OUTPUT_LOG)
	@complexity --score --thresh=$(COMPLEXITY_THRESHOLD) $(subst _test.out,.c, $@)	| tee -a $(OUTPUT_LOG)
	@echo																			| tee -a $(OUTPUT_LOG)
	@printf "+-----------------------------+\n"										| tee -a $(OUTPUT_LOG)
	@printf "| Unit test coverage          |\n"										| tee -a $(OUTPUT_LOG)
	@printf "+-----------------------------+\n"										| tee -a $(OUTPUT_LOG)
	@echo																			| tee -a $(OUTPUT_LOG)
	@gcov $(subst _test.out,_test.c, $@)											| tee -a $(OUTPUT_LOG)
	@echo																			| tee -a $(OUTPUT_LOG)
	@echo																			| tee -a $(OUTPUT_LOG)

#####################################################################################
# Run the unit test and print to file												#
#####################################################################################
pico_mqtt_functional_test.out: pico_mqtt_functional_test.o $(BASE_OBJECT_FILES) pico_mqtt_debug.o
	@$(CC) $(C_FLAGS) $^ -o $@ $(TEST_FLAGS) $(COVERAGE_FLAGS) $(DEBUG_FLAGS)


#####################################################################################
# Cleaning up the generated files 													#
#####################################################################################

clean:
	rm -f *.$(EXTENSION) *.o *.gcov *.gcda *.gcno $(DEPDIR)/* *.out $(OUTPUT_LOG) $(OUTPUT_LOG_TMP)
	rmdir $(DEPDIR)