TARGET=mode_agent
TARGET_MGMT_SCRIPT=mode_agent_mgmt

CURRENT_DIR=$(shell pwd)
TARGET_INSTALL_DIR=/sbin
TARGET_INSTALL_SCRIPT_DIR=/etc/init.d

CC=g++

CFLAGS=-c -std=c++14 -Wall -Wextra -Werror -pedantic -I.

LFLAGS =
LIBDEP = -pthread

SOURCES=main.cpp \
	mode_ctrl.cpp

OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) $(TARGET)

install:
	cp $(CURRENT_DIR)/$(TARGET) $(TARGET_INSTALL_DIR)
	cp $(CURRENT_DIR)/$(TARGET_MGMT_SCRIPT) $(TARGET_INSTALL_SCRIPT_DIR)

uninstall:
	rm -f $(TARGET_INSTALL_DIR)/$(TARGET)
	rm -f $(TARGET_INSTALL_SCRIPT_DIR)/$(TARGET_MGMT_SCRIPT)

$(TARGET): $(OBJECTS)
	$(CC) $(LFLAGS) $(LIBDEP) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(TARGET)
