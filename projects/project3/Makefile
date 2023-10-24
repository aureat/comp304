TARGETS = virtmem part1 part2

CC := cc

SRC_DIR := .
BUILD_DIR := ./build
DEP_DIR := $(BUILD_DIR)/.deps

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS := $(patsubst $(SRC_DIR)/%.c, $(DEP_DIR)/%.d, $(SRCS))

WARN_FLAGS += -Wno-error 
MAKE_FLAGS += -j
DEP_FLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d
CFLAGS += $(WARN_FLAGS) -std=c17

INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

.MAIN: $(TARGET_EXEC)

virtmem: $(BUILD_DIR)/virtmem.o
	$(CC) $(BUILD_DIR)/virtmem.o -o $@ $(LDFLAGS)

part1: $(BUILD_DIR)/part1.o
	$(CC) $(BUILD_DIR)/part1.o -o $@ $(LDFLAGS)

part2: $(BUILD_DIR)/part2.o
	$(CC) $(BUILD_DIR)/part2.o -o $@ $(LDFLAGS)

tester: $(BUILD_DIR)/tester.o
	$(CC) $(BUILD_DIR)/tester.o -o $@ $(LDFLAGS)

all: $(TARGETS) $(OBJ)

$(OBJS) : $(BUILD_DIR)/%.o : $(SRC_DIR)/%.c $(DEP_DIR)/%.d | $(DEP_DIR)
	@mkdir -p "$(dir $(DEP_DIR)/$*)"
	@mkdir -p $(@D)
	$(CC) $(INC_FLAGS) $(CFLAGS) $(DEP_FLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(TARGET_EXEC)
	$(RM) -rd $(BUILD_DIR)
	$(RM) $(TARGETS)

.PHONY: format
format: $(SRCS)
	clang-format --style=file:.clang-format -i $^

$(DEP_DIR):
	@mkdir -p $(DEP_DIR)

$(DEPS):

-include $(wildcard $(DEPS))

.PHONY: help
help:
	@echo  'Targets:'
	@echo  '  all               - Compiles everything'
	@echo  '  virtmem           - Compiles virtmem base code'
	@echo  '  part1             - Compiles part1'
	@echo  '  part2             - Compiles part2'
	@echo  ''
	@echo  '  clean             - Removes build files'
	@echo  ''
	@echo  '  format            - Formats code'
