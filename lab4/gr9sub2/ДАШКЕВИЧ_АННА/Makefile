CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11

BIN_DIR := bin
SRC_DIR := src
LOG_DIR := logs

PROFILER := $(BIN_DIR)/memory_profiler
VMAN := $(BIN_DIR)/vm_analyzer
IOBENCH := $(BIN_DIR)/io_benchmark
IOMON := $(BIN_DIR)/io_monitor

all: prep $(PROFILER) $(VMAN) $(IOBENCH) $(IOMON)

prep:
	mkdir -p $(BIN_DIR) $(LOG_DIR)

$(PROFILER): $(SRC_DIR)/memory_profiler.c
	$(CC) $(CFLAGS) -o $@ $<

$(VMAN): $(SRC_DIR)/vm_analyzer.c
	$(CC) $(CFLAGS) -o $@ $<

$(IOBENCH): $(SRC_DIR)/io_benchmark.c
	$(CC) $(CFLAGS) -o $@ $<

$(IOMON): $(SRC_DIR)/io_monitor.c
	$(CC) $(CFLAGS) -o $@ $<

run_profiler:
	@echo "Usage: make run_profiler PID=<pid> [WATCH=1]"
	@if [ -z "$(PID)" ]; then echo "Provide PID, e.g.: make run_profiler PID=$$PPID"; exit 1; fi
	@if [ "$(WATCH)" = "1" ]; then \
		$(PROFILER) --watch $(PID) | tee $(LOG_DIR)/profiler_watch_$(PID).log; \
	else \
		$(PROFILER) $(PID) | tee $(LOG_DIR)/profiler_$(PID).log; \
	fi

run_vman:
	$(VMAN) | tee $(LOG_DIR)/vm_analyzer.log

run_iobench:
	$(IOBENCH) | tee $(LOG_DIR)/io_benchmark.log

run_iomon:
	$(IOMON) | tee $(LOG_DIR)/io_monitor.log

clean:
	rm -rf $(BIN_DIR) $(LOG_DIR)/*.log test_*.bin