TARGET=mode_agent_cli

CC=g++

CFLAGS=-c -std=c++14 -Wall -Wextra -Werror -pedantic -I.

LFLAGS =
LIBDEP = -pthread

SOURCES=main.cpp

OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LFLAGS) $(LIBDEP) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(TARGET)
