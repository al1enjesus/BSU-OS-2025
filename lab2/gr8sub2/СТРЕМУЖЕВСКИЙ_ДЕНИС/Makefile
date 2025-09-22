APP1     := fork_example
SRC1     := src/fork_example.c

APP2     := ptree
SRC2     := src/ptree.c

CC       ?= cc
CFLAGS   ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
LDFLAGS  ?=

DEBUG_FLAGS := -O0 -g
SAN_FLAGS   := -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: all run run-ptree clean debug sanitize

all: $(APP1) $(APP2)

$(APP1): $(SRC1)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(APP2): $(SRC2)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

run-fork: $(APP1)
	./$(APP1)

run-ptree: $(APP2)
	./$(APP2)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all

sanitize: CFLAGS += $(DEBUG_FLAGS) $(SAN_FLAGS)
sanitize: LDFLAGS += $(SAN_FLAGS)
sanitize: clean all

clean:
	$(RM) $(APP1) $(APP2)
