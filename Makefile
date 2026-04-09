CC ?= gcc
PKGS = gtk+-3.0 webkit2gtk-4.1
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -g $(shell pkg-config --cflags $(PKGS))
LDLIBS += $(shell pkg-config --libs $(PKGS))

BUILD_DIR = build
TARGET = $(BUILD_DIR)/kitekit-browser
SRC = src/main.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

$(BUILD_DIR):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET) $(URL)

clean:
	rm -rf $(BUILD_DIR)
