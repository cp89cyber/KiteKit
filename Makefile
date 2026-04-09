CC ?= gcc
APP_PKGS = gtk+-3.0 webkit2gtk-4.1
TEST_PKGS = gtk+-3.0
CPPFLAGS += -Isrc
COMMON_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g
APP_CFLAGS = $(COMMON_CFLAGS) $(shell pkg-config --cflags $(APP_PKGS))
TEST_CFLAGS = $(COMMON_CFLAGS) $(shell pkg-config --cflags $(TEST_PKGS))
APP_LDLIBS = $(shell pkg-config --libs $(APP_PKGS))
TEST_LDLIBS = $(shell pkg-config --libs $(TEST_PKGS))

BUILD_DIR = build
TARGET = $(BUILD_DIR)/kitekit-browser
TEST_TARGET = $(BUILD_DIR)/runtime_policy_test
SRC = src/main.c src/runtime.c
TEST_SRC = tests/runtime_policy.c src/runtime.c

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(APP_CFLAGS) -o $@ $(SRC) $(APP_LDLIBS)

$(TEST_TARGET): $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(TEST_CFLAGS) -o $@ $(TEST_SRC) $(TEST_LDLIBS)

$(BUILD_DIR):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET) $(URL)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR)
