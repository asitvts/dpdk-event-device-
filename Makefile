# App name
APP = main

# Source files
SRCS = main.c

# Compiler
CC ?= gcc

# DPDK flags via pkg-config
CFLAGS += $(shell pkg-config --cflags libdpdk)
LDFLAGS += $(shell pkg-config --libs libdpdk)

# Optional but common
CFLAGS += -O3 -Wall -Wextra

all: $(APP)

$(APP): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(APP)

.PHONY: all clean
