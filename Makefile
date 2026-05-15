# Makefile - test scenarios for process-monitor
#
# Targets:
#  test-perf      - performance test: run `./build/process_monitor --once` N times
#                   and fail if average runtime exceeds EXPECT_MAX (seconds)
#  test-terminate - correctness test: start a background `sleep`, run the app
#                   to issue `kill <pid>` (via piped commands) and verify the
#                   process no longer exists.
#  test           - run both tests

.PHONY: all test test-perf test-terminate
all: test

TEST_RUNS ?= 5
EXPECT_MAX ?= 1.0

BIN = build/process_monitor

test: test-perf test-terminate

test-perf:
	@echo "Running performance test: $(BIN) --once ($(TEST_RUNS) runs)"
	@./scripts/test_perf.sh $(TEST_RUNS) $(EXPECT_MAX)

test-terminate:
	@echo "Running termination correctness test"
	@if [ ! -x "$(BIN)" ]; then echo "Executable $(BIN) not found. Build first."; exit 2; fi
	@echo "Spawning a background sleep process..."
	@./scripts/test_terminate.sh
